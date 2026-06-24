#pragma once
#include "leanffi.h"
#include "result.h"
#include <string>
#include <vector>
#include <random>
#include <optional>

namespace lpi {

// Spec §4.1 — Random Lean File Test Sampling.
//
// We pick a random `.lean` file from /root/mycode/lean4, hand it to
// `LeanFFI::run_file` (or `run_source(read(f))`), and record the
// outcome as evidence. Determinism is preserved by carrying a
// `seed` value; the same seed always produces the same file pick.
struct LeanFileSample {
    std::string file_path;
    std::string file_hash;
    std::string source;             // truncated for evidence
    bool load_ok = false;
    bool kernel_ok = false;
    std::string error;
    std::string kernel_response;
    std::string evidence_path;
};

class LeanFileSampler {
public:
    explicit LeanFileSampler(const std::string& root = "/root/mycode/lean4");

    // Discover every .lean file under root.
    size_t index_files();
    size_t index_size() const { return files_.size(); }

    // Deterministic pick (no execution). Empty if index empty.
    std::string pick(uint64_t seed) const;

    // Deterministic file pick + LeanFFI execution. Writes an evidence
    // row under evidence/test_sampling/<ts>_<hash>.json. Returns false
    // only on infrastructure errors (no files indexed, I/O failure).
    bool sample_and_run(uint64_t seed, LeanFFI& ffi, LeanFileSample& out);

    // Override file path; same semantics as above.
    bool sample_and_run_specific(const std::string& abs_path,
                                 LeanFFI& ffi,
                                 LeanFileSample& out);

    // Static helpers.
    static std::string fingerprint(const std::string& s);
    static std::string truncate(const std::string& s, size_t max_bytes = 2048);

private:
    std::string root_;
    std::vector<std::string> files_;
};

// Spec §4.2 — addTheorem / addLemma Random Test Generation.
enum class FfiGenKind {
    THEOREM,
    LEMMA,
};

struct ExtractedContext {
    std::vector<std::string> imports;
    std::vector<std::string> definitions;
    std::vector<std::string> structures;
    std::vector<std::string> theorems;
    std::string first_type;   // candidate for the synthesised type
};

struct FfiGenSample {
    std::string source_path;
    std::string file_hash;
    ExtractedContext ctx;
    FfiGenKind kind = FfiGenKind::THEOREM;
    std::string generated_snippet;
    std::string generated_name;
    std::string generated_type;
    std::string generated_proof;
    bool kernel_ok = false;
    std::string error;
    std::string kernel_response;
    std::string evidence_path;
};

class FfiGenSynthesizer {
public:
    explicit FfiGenSynthesizer(LeanFileSampler& sampler);

    bool generate_and_record(uint64_t seed,
                             FfiGenKind kind,
                             FfiGenSample& out);

    bool generate_specific(const std::string& abs_path,
                           FfiGenKind kind,
                           FfiGenSample& out);

    bool run_against(LeanFFI& ffi, FfiGenSample& s);

    // Pure helpers (exposed for tests and reuse).
    static ExtractedContext extract(const std::string& source);
    static std::string synthesize_theorem(const ExtractedContext& ctx,
                                          FfiGenKind kind,
                                          std::string& out_name,
                                          std::string& out_type,
                                          std::string& out_proof);
    static std::string fingerprint(const std::string& s);

private:
    LeanFileSampler& sampler_;
};

}  // namespace lpi
