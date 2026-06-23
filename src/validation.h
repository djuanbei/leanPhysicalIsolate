#pragma once
#include <string>
#include <vector>
#include <optional>

namespace lpi {

// Validation framework (spec §18).
//
// Each check produces a row in evidence/validation.json with:
//   { "check": "...", "result": "pass"|"fail", "details": {...} }
class Validation {
public:
    struct Result {
        std::string check;
        bool ok = false;
        std::string details_json;
    };

    // 1. semantic correctness: repl versions and a representative
    //    elaboration match a known-good baseline.
    static Result semantic_correctness(const std::string& repl_path);

    // 2. isolation integrity: scan all instance_<id>/ directories and
    //    assert no two share an inode table (best-effort: distinct dirs).
    static Result isolation_integrity();

    // 3. memory usage: current vs baseline.
    static Result memory_check();

    // 4. runtime throughput: evaluations/sec over a window.
    static Result throughput_check(size_t evaluations, double seconds);

    // 5. snapshot correctness: capture, restore, then check the env archive
    //    round-trips.
    static Result snapshot_correctness(const std::string& repl_path);

    // 6. semantic mismatch: ensure LeanFFI behaviour matches the Lean
    //    kernel (we delegate to the repl, which IS the kernel).
    static Result kernel_semantic_match(const std::string& repl_path);
};

}  // namespace lpi
