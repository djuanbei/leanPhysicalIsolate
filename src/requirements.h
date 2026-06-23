#pragma once
#include <string>
#include <vector>
#include <optional>

namespace lpi {

// Requirements system (spec §4).
//
// Requirements live in /root/mycode/lean_physical_isolate/requirements/<area>/*.md.
// They are loaded, validated against the implementation, and the outcome
// is logged + committed. Lifecycle:
//
//     Edit -> Validate -> Evidence -> Log -> git commit
struct Requirement {
    std::string id;          // e.g. "core/REQ-001"
    std::string area;        // e.g. "core"
    std::string title;
    std::string body;
    std::string path;
    bool satisfied = false;
};

class Requirements {
public:
    static Requirements& get();

    // Walk /root/mycode/lean_physical_isolate/requirements/ and load all .md.
    std::vector<Requirement> load_all();

    // Compute the satisfied flag for a single requirement.
    // We use a tiny DSL inside the markdown body: lines starting with
    // `check:` name a Validation check to run; `expect: pass|fail`
    // sets the expected outcome.
    bool evaluate(const Requirement& req);

    // Run evaluate for every requirement and write evidence/requirements.jsonl
    void evaluate_all();

private:
    Requirements() = default;
    std::string root_ = "/root/mycode/lean_physical_isolate/requirements";
};

}  // namespace lpi
