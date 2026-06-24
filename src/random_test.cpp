#include "random_test.h"
#include "json_helpers.h"
#include "logger.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <random>
#include <unordered_set>

namespace fs = std::filesystem;

namespace lpi {

// ---------------------------------------------------------------------------
// Tiny non-crypto 64-bit hash (xxhash-style mix). Sufficient for fingerprinting
// evidence files; we don't need cryptographic collision resistance.
// ---------------------------------------------------------------------------
static uint64_t mix64(uint64_t x) {
    x ^= x >> 33; x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

std::string LeanFileSampler::fingerprint(const std::string& s) {
    uint64_t h1 = 0xcbf29ce484222325ULL;
    uint64_t h2 = 0x84222325cbf29ce4ULL;
    for (size_t i = 0; i < s.size(); i += 16) {
        uint64_t chunk = 0;
        size_t end = std::min<size_t>(s.size(), i + 16);
        for (size_t j = i; j < end; ++j) {
            chunk = (chunk << 8) | (unsigned char)s[j];
        }
        h1 = mix64(h1 ^ chunk);
        h2 = mix64(h2 + chunk);
    }
    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << h1
        << std::hex << std::setw(16) << std::setfill('0') << h2;
    return oss.str();
}

std::string LeanFileSampler::truncate(const std::string& s, size_t max_bytes) {
    if (s.size() <= max_bytes) return s;
    return s.substr(0, max_bytes) + "...[truncated " +
           std::to_string(s.size() - max_bytes) + " bytes]";
}

LeanFileSampler::LeanFileSampler(const std::string& root) : root_(root) {}

size_t LeanFileSampler::index_files() {
    files_.clear();
    std::error_code ec;
    if (!fs::exists(root_, ec) || !fs::is_directory(root_, ec)) {
        return 0;
    }
    for (auto& e : fs::recursive_directory_iterator(root_, ec)) {
        if (ec) break;
        if (!e.is_regular_file()) continue;
        if (e.path().extension() != ".lean") continue;
        // Skip .lake / build trees — they're build artifacts, not source.
        const std::string s = e.path().string();
        if (s.find("/.lake/") != std::string::npos) continue;
        if (s.find("/build/") != std::string::npos) continue;
        if (s.find("/stage0/") != std::string::npos) continue;
        files_.push_back(s);
    }
    std::sort(files_.begin(), files_.end());
    files_.erase(std::unique(files_.begin(), files_.end()), files_.end());
    return files_.size();
}

std::string LeanFileSampler::pick(uint64_t seed) const {
    if (files_.empty()) return std::string();
    uint64_t z = seed + 0x9e3779b97f4a7c15ULL;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    z = z ^ (z >> 31);
    size_t idx = (size_t)(z % (uint64_t)files_.size());
    return files_[idx];
}

static std::string iso_compact_now() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                  now.time_since_epoch()) % 1'000'000;
    std::tm tm{};
    gmtime_r(&t, &tm);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%dT%H%M%S")
        << std::setw(6) << std::setfill('0') << us.count();
    return oss.str();
}

bool LeanFileSampler::sample_and_run(uint64_t seed, LeanFFI& ffi, LeanFileSample& out) {
    if (files_.empty() && index_files() == 0) {
        out.error = "no .lean files indexed under " + root_;
        return false;
    }
    // Deterministic pick: splitmix64 on the user-supplied seed.
    uint64_t z = seed + 0x9e3779b97f4a7c15ULL;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    z = z ^ (z >> 31);
    size_t idx = (size_t)(z % (uint64_t)files_.size());
    return sample_and_run_specific(files_[idx], ffi, out);
}

bool LeanFileSampler::sample_and_run_specific(const std::string& abs_path,
                                               LeanFFI& ffi,
                                               LeanFileSample& out) {
    out.file_path = abs_path;
    out.load_ok = false;
    out.kernel_ok = false;
    out.error.clear();
    out.kernel_response.clear();
    out.evidence_path.clear();

    std::ifstream in(abs_path);
    if (!in) {
        out.error = "cannot open file";
        return false;
    }
    std::stringstream ss; ss << in.rdbuf();
    std::string src = ss.str();
    out.source = truncate(src);
    out.file_hash = fingerprint(src);

    // Delegate to LeanFFI: real kernel runs the source verbatim via
    // run_file. We treat "kernel returned a result with no error field"
    // as kernel_ok; any "error" or transport failure is kernel rejection.
    Result r = ffi.run_file(abs_path);
    out.load_ok = r.ok;
    if (!r.ok) {
        out.error = r.error;
        out.kernel_response = truncate(r.raw_json.empty() ? std::string("{}") : r.raw_json);
    } else {
        out.kernel_ok = true;
        out.kernel_response = truncate(r.raw_json);
    }

    // Write evidence row.
    fs::create_directories("evidence/test_sampling");
    std::string ts = iso_compact_now();
    std::string ev = "evidence/test_sampling/" + ts + "_" +
                     out.file_hash.substr(0, 16) + ".json";
    {
        std::ofstream os(ev);
        os << "{\n";
        os << "  \"spec\": \"4.1\",\n";
        os << "  \"ts\": \"" << iso8601_now() << "\",\n";
        os << "  \"file_path\": \"" << json::escape(out.file_path) << "\",\n";
        os << "  \"file_hash\": \"" << out.file_hash << "\",\n";
        os << "  \"load_ok\": " << (out.load_ok ? "true" : "false") << ",\n";
        os << "  \"kernel_ok\": " << (out.kernel_ok ? "true" : "false") << ",\n";
        os << "  \"error\": \"" << json::escape(out.error) << "\",\n";
        os << "  \"source\": \"" << json::escape(out.source) << "\",\n";
        os << "  \"kernel_response\": \"" << json::escape(out.kernel_response) << "\"\n";
        os << "}\n";
    }
    out.evidence_path = ev;
    Logger::get().event(ffi.instance_id(), "random_lean_file_test",
                        ev, out.kernel_ok ? "pass" : "fail",
                        std::string("{\"file\":\"") + json::escape(out.file_path) +
                        "\",\"hash\":\"" + out.file_hash + "\"}");
    return true;
}

// ===========================================================================
// FfiGenSynthesizer
// ===========================================================================

FfiGenSynthesizer::FfiGenSynthesizer(LeanFileSampler& sampler) : sampler_(sampler) {}

std::string FfiGenSynthesizer::fingerprint(const std::string& s) {
    return LeanFileSampler::fingerprint(s);
}

static std::string strip_comment(const std::string& s) {
    // strip `--` line comments, keep /* ... */ for now (rare in Lean).
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ) {
        if (i + 1 < s.size() && s[i] == '-' && s[i+1] == '-') {
            while (i < s.size() && s[i] != '\n') ++i;
            continue;
        }
        out.push_back(s[i++]);
    }
    return out;
}

ExtractedContext FfiGenSynthesizer::extract(const std::string& source) {
    ExtractedContext ctx;
    std::unordered_set<std::string> seen;

    std::string src = strip_comment(source);
    std::istringstream iss(src);
    std::string line;

    while (std::getline(iss, line)) {
        std::string trimmed = line;
        // trim leading whitespace
        size_t a = 0;
        while (a < trimmed.size() && (trimmed[a] == ' ' || trimmed[a] == '\t')) ++a;
        size_t b = trimmed.size();
        while (b > a && (trimmed[b-1] == ' ' || trimmed[b-1] == '\t' ||
                         trimmed[b-1] == '\r')) --b;
        std::string t = trimmed.substr(a, b - a);
        if (t.empty()) continue;

        // import Foo.Bar.Baz  /  import Foo
        if (t.rfind("import ", 0) == 0) {
            std::string imp = t.substr(7);
            // strip trailing "..." or whitespace
            size_t sp = imp.find_first_of(" \t");
            if (sp != std::string::npos) imp = imp.substr(0, sp);
            if (!imp.empty() && seen.insert("I:" + imp).second) {
                ctx.imports.push_back(imp);
            }
            continue;
        }

        // open namespace Foo / end Foo
        if (t.rfind("namespace ", 0) == 0 || t.rfind("section ", 0) == 0) continue;
        if (t.rfind("end ", 0) == 0 || t == "end") continue;

        // structure / class
        if (t.rfind("structure ", 0) == 0 || t.rfind("class ", 0) == 0) {
            std::string decl = t;
            // multi-line structure ... where\n ... ;\n end
            if (decl.find("where") != std::string::npos ||
                decl.find("extends") != std::string::npos) {
                // capture until end of line, allow up to 8 following lines.
                std::string block = decl + "\n";
                for (int k = 0; k < 8 && std::getline(iss, line); ++k) {
                    block += line + "\n";
                    if (line.find(';') != std::string::npos) break;
                }
                if (seen.insert("S:" + block).second) ctx.structures.push_back(block);
            } else {
                if (seen.insert("S:" + decl).second) ctx.structures.push_back(decl);
            }
            continue;
        }

        // theorem / lemma
        if (t.rfind("theorem ", 0) == 0 || t.rfind("lemma ", 0) == 0 ||
            t.rfind("example ", 0) == 0) {
            if (seen.insert("T:" + t).second) ctx.theorems.push_back(t);
            // extract the type clause "name : TYPE"
            size_t colon = t.find(" : ");
            if (colon != std::string::npos) {
                size_t eq = t.find(":=", colon);
                std::string typ = (eq == std::string::npos)
                    ? t.substr(colon + 3)
                    : t.substr(colon + 3, eq - colon - 3);
                // trim trailing
                while (!typ.empty() && (typ.back() == ' ' || typ.back() == '\t' ||
                                        typ.back() == '{')) typ.pop_back();
                if (!typ.empty() && ctx.first_type.empty()) {
                    // limit to first line
                    size_t nl = typ.find('\n');
                    if (nl != std::string::npos) typ = typ.substr(0, nl);
                    ctx.first_type = typ;
                }
            }
            continue;
        }

        // def / abbrev
        if (t.rfind("def ", 0) == 0 || t.rfind("abbrev ", 0) == 0) {
            if (seen.insert("D:" + t).second) ctx.definitions.push_back(t);
            continue;
        }
    }
    return ctx;
}

std::string FfiGenSynthesizer::synthesize_theorem(const ExtractedContext& ctx,
                                                   FfiGenKind kind,
                                                   std::string& out_name,
                                                   std::string& out_type,
                                                   std::string& out_proof) {
    // Pick a name based on a hash of the file context so different files
    // produce different theorem names (no global collisions across runs).
    std::string seed_src;
    for (auto& s : ctx.imports) seed_src += s + "\n";
    for (auto& s : ctx.theorems) seed_src += s + "\n";
    for (auto& s : ctx.definitions) seed_src += s + "\n";
    std::string fp = fingerprint(seed_src);
    std::string kind_tag = (kind == FfiGenKind::LEMMA) ? "lpi_lem_" : "lpi_thm_";
    out_name = kind_tag + fp.substr(0, 10);
    out_type = ctx.first_type;
    // Heuristic: only use the extracted type when it looks like a
    // simple propositional statement (begins with a name-like token
    // and does not mention constructs that would require unresolvable
    // imports — e.g. `f1 1 = 2` from a file that defines `f1`).  When
    // the type looks like it depends on locally-bound constants, we
    // fall back to `True` so the generated injection actually exercises
    // the kernel's theorem/lemma-acceptance code path.
    auto looks_local = [](const std::string& t) {
        if (t.empty()) return true;
        // Local helpers in the corpus are typically short lower-case
        // names with digits; `True`, `False`, `Prop`, `Nat` etc. are
        // public constants from Init.
        size_t i = 0;
        // Skip leading whitespace
        while (i < t.size() && (t[i] == ' ' || t[i] == '\t')) ++i;
        // Reject if it starts with a lowercase identifier that contains
        // digits — that's the typical signature of a local helper.
        if (i < t.size() && std::islower((unsigned char)t[i])) {
            for (size_t j = i; j < t.size() && (std::isalnum((unsigned char)t[j]) || t[j] == '_'); ++j) {
                if (std::isdigit((unsigned char)t[j])) return true;
            }
        }
        return false;
    };
    if (out_type.empty() || looks_local(out_type)) {
        out_type = "True";
    }
    out_proof = "by trivial";

    // If we have imports, prepend them; otherwise use the Init prelude
    // the repl already loaded.
    std::ostringstream os;
    for (auto& imp : ctx.imports) {
        // Validate it's a clean module path (no `..`, no `*`).
        if (imp.find_first_of(" *()[]{}<>,;\\\"'") != std::string::npos) continue;
        if (imp.empty() || imp[0] == '.') continue;
        os << "import " << imp << "\n";
    }
    std::string kw = (kind == FfiGenKind::LEMMA) ? "lemma " : "theorem ";
    os << kw << out_name << " : " << out_type << " := " << out_proof << "\n";
    return os.str();
}

bool FfiGenSynthesizer::generate_and_record(uint64_t seed,
                                            FfiGenKind kind,
                                            FfiGenSample& out) {
    if (sampler_.index_size() == 0 && sampler_.index_files() == 0) {
        out.error = "no .lean files indexed";
        return false;
    }
    std::string path = sampler_.pick(seed);
    if (path.empty()) {
        out.error = "sampler pick returned empty";
        return false;
    }
    return generate_specific(path, kind, out);
}

bool FfiGenSynthesizer::generate_specific(const std::string& abs_path,
                                          FfiGenKind kind,
                                          FfiGenSample& out) {
    out.source_path = abs_path;
    out.kind = kind;
    out.kernel_ok = false;
    out.error.clear();
    out.kernel_response.clear();
    out.evidence_path.clear();

    std::ifstream in(abs_path);
    if (!in) {
        out.error = "cannot open file";
        return false;
    }
    std::stringstream ss; ss << in.rdbuf();
    std::string src = ss.str();
    out.file_hash = fingerprint(src);
    out.ctx = extract(src);
    out.generated_snippet = synthesize_theorem(out.ctx, kind,
                                               out.generated_name,
                                               out.generated_type,
                                               out.generated_proof);
    return true;
}

bool FfiGenSynthesizer::run_against(LeanFFI& ffi, FfiGenSample& s) {
    if (s.generated_snippet.empty()) {
        s.error = "empty snippet; call generate_specific first";
        return false;
    }
    // Spec §4.2 + §14: the kernel's addTheorem / addLemma injection
    // path is the `env.add` protocol call. We drive it directly with
    // the synthesised (name, type, value) triple instead of round-
    // tripping a free-form source through expr.echo (which only
    // elaborates terms, not declarations).
    bool is_thm = (s.kind == FfiGenKind::THEOREM);
    Result r = ffi.env_add_raw(s.generated_name,
                               s.generated_type,
                               s.generated_proof,
                               is_thm);
    if (r.ok) {
        s.kernel_ok = true;
        s.kernel_response = LeanFileSampler::truncate(r.raw_json);
    } else {
        s.error = r.error;
        s.kernel_response = LeanFileSampler::truncate(
            r.raw_json.empty() ? std::string("{}") : r.raw_json);
    }

    fs::create_directories("evidence/ffi_generated");
    std::string ts = iso_compact_now();
    std::string ev = "evidence/ffi_generated/" + ts + "_" +
                     s.file_hash.substr(0, 16) + ".json";
    {
        std::ofstream os(ev);
        os << "{\n";
        os << "  \"spec\": \"4.2\",\n";
        os << "  \"ts\": \"" << iso8601_now() << "\",\n";
        os << "  \"file_path\": \"" << json::escape(s.source_path) << "\",\n";
        os << "  \"file_hash\": \"" << s.file_hash << "\",\n";
        os << "  \"kind\": \"" << (s.kind == FfiGenKind::LEMMA ? "lemma" : "theorem") << "\",\n";
        os << "  \"name\": \"" << json::escape(s.generated_name) << "\",\n";
        os << "  \"type\": \"" << json::escape(s.generated_type) << "\",\n";
        os << "  \"proof\": \"" << json::escape(s.generated_proof) << "\",\n";
        os << "  \"kernel_ok\": " << (s.kernel_ok ? "true" : "false") << ",\n";
        os << "  \"error\": \"" << json::escape(s.error) << "\",\n";
        os << "  \"generated_snippet\": \"" << json::escape(s.generated_snippet) << "\",\n";
        os << "  \"imports\": [";
        for (size_t i = 0; i < s.ctx.imports.size(); ++i) {
            if (i) os << ",";
            os << "\"" << json::escape(s.ctx.imports[i]) << "\"";
        }
        os << "],\n";
        os << "  \"kernel_response\": \"" << json::escape(s.kernel_response) << "\"\n";
        os << "}\n";
    }
    s.evidence_path = ev;
    Logger::get().event(ffi.instance_id(), "ffi_generated_test",
                        ev, s.kernel_ok ? "pass" : "fail",
                        std::string("{\"name\":\"") + s.generated_name +
                        "\",\"file_hash\":\"" + s.file_hash + "\"}");
    return true;
}

}  // namespace lpi
