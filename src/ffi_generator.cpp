#include "ffi_generator.h"
#include "util.h"
#include <regex>
#include <set>
#include <sstream>

namespace leanffi {

bool FfiGenerator::is_valid_ident(const std::string& s) const {
    if (s.empty()) return false;
    if (!(std::isalpha(static_cast<unsigned char>(s[0])) || s[0] == '_')) return false;
    for (char c : s) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.')) return false;
    }
    return true;
}

std::string FfiGenerator::clean_ident(const std::string& s) const {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') out += c;
        else if (c == ' ' || c == '\t') out += '_';
    }
    if (out.empty()) return "auto_leanffi_decl";
    if (!(std::isalpha(static_cast<unsigned char>(out[0])) || out[0] == '_')) out = "_" + out;
    return out;
}

std::string FfiGenerator::clean_type(const std::string& s) const {
    // collapse whitespace
    std::string out;
    bool prev_ws = false;
    for (char c : s) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!prev_ws && !out.empty()) out += ' ';
            prev_ws = true;
        } else {
            out += c;
            prev_ws = false;
        }
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    if (out.size() > 240) out.resize(240);
    return out;
}

static bool looks_like_theorem_kw(const std::string& kw) {
    return kw == "theorem" || kw == "lemma" || kw == "example";
}

std::vector<FfiTest> FfiGenerator::generate(const SampledFile& file, size_t max_per_file) const {
    std::vector<FfiTest> out;
    if (file.source.empty()) return out;

    // regex matches `theorem foo ... : type := proof` or `lemma foo ... : type := proof`
    // captures: 1=keyword, 2=name, 3=binders (optional), 4=type, 5=proof
    std::regex re_decl(R"((theorem|lemma)\s+([A-Za-z_][A-Za-z0-9_'.]*)\s*(\{[^}]*\}|\([^)]*\))?\s*:\s*([^:=]+?)\s*:=\s*([^\n]+))");
    auto begin = std::sregex_iterator(file.source.begin(), file.source.end(), re_decl);
    auto end = std::sregex_iterator();

    std::set<std::string> seen;
    for (auto it = begin; it != end && out.size() < max_per_file; ++it) {
        std::smatch m = *it;
        std::string kw = m[1].str();
        std::string name = m[2].str();
        std::string binders = m[3].matched ? m[3].str() : "";
        std::string type = m[4].str();
        std::string proof = m[5].str();

        std::string clean_name = clean_ident(name);
        if (!is_valid_ident(clean_name) || seen.count(clean_name)) continue;

        // strip trailing proof tokens (commas, periods)
        while (!proof.empty() && (proof.back() == ',' || proof.back() == ';' ||
                                  std::isspace(static_cast<unsigned char>(proof.back()))))
            proof.pop_back();

        std::string full_type = trim(type);
        if (!binders.empty()) {
            std::string b = binders;
            if (b.size() >= 2 && b.front() == '(' && b.back() == ')') b = b.substr(1, b.size() - 2);
            else if (b.size() >= 2 && b.front() == '{' && b.back() == '}') b = b.substr(1, b.size() - 2);
            if (!b.empty()) full_type = "forall " + b + ", " + full_type;
        }
        full_type = clean_type(full_type);
        if (full_type.empty()) continue;
        if (full_type.size() > 240) continue;
        // avoid proofs that require non-builtin definitions
        bool safe_proof = (proof.find("by sorry") != std::string::npos) ||
                          (proof == "rfl") ||
                          (proof == "True.intro") ||
                          (proof.size() <= 24 && proof.find_first_of(":=;{}[]<>\\") == std::string::npos);
        if (!safe_proof) continue;
        seen.insert(clean_name);

        FfiTest t;
        t.kind = looks_like_theorem_kw(kw) ? (kw == "lemma" ? "addLemma" : "addTheorem") : "addTheorem";
        t.name = clean_name;
        t.type = full_type;
        t.value = "by sorry";
        t.source_file = file.rel_path;
        t.source_hash = file.hash;
        out.push_back(std::move(t));
    }
    return out;
}

}