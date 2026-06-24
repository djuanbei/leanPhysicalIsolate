#pragma once
#include <string>
#include <vector>
#include "json_min.h"

namespace leanffi {

struct ValidationCheck {
    std::string name;
    bool passed = false;
    std::string detail;
    JsonValue extra = JsonValue(JsonObject{});
};

class ValidationFramework {
public:
    // Run all required validation checks and write evidence
    // `instance_count`: number of instances currently active
    // returns true if all critical checks pass
    bool run_all(size_t instance_count,
                 size_t expected_instances,
                 double elapsed_seconds,
                 uint64_t total_evaluations);

    // individual checks (run independently by main pipeline)
    ValidationCheck check_isolation(const std::string& runtime_root);
    ValidationCheck check_evidence(const std::string& evidence_root);
    ValidationCheck check_snapshots(const std::string& snapshots_root);
    ValidationCheck check_pantograph_dependency(const std::string& pantograph_root,
                                               const std::string& build_dir);
    ValidationCheck check_random_lean_file_executed(const std::string& evidence_root);
    ValidationCheck check_ffi_generation(const std::string& evidence_root);

    const std::vector<ValidationCheck>& results() const { return results_; }
    bool all_passed() const;

private:
    std::vector<ValidationCheck> results_;
};

}