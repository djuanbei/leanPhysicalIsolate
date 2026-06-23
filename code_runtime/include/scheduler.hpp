// Scheduler: dispatches tasks to a bounded pool of LeanFFI instances.
// Stateless or bounded state only; streams results to a consumer callback.

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <atomic>

#include "leanffi.hpp"

namespace leanffi {

class LeanFFI;  // forward declaration

enum class Policy {
    ROUND_ROBIN,
    LEAST_LOAD,
    AFFINITY,
    DAG_AWARE,
};

struct DispatchResult {
    int64_t task_id = 0;
    int64_t instance_id = -1;
    bool success = false;
    std::string output;
    std::string error;
    double wall_seconds = 0.0;
    int64_t wall_ms = 0;
};

struct SchedulerStats {
    int64_t total = 0;
    int64_t succeeded = 0;
    int64_t failed = 0;
    double total_wall_seconds = 0.0;
    double eval_per_sec = 0.0;
};

// Stateless scheduler: given a pool of instances and a list of tasks,
// dispatches each task to the next instance per the policy. Tasks run
// serially per instance (one Pantograph REPL is single-threaded in this
// design), so total wall time ≈ sum of per-instance task times.
class Scheduler {
public:
    Scheduler(Policy p) : policy_(p) {}

    // Run all tasks on the given instance pool. The pool is borrowed;
    // the scheduler does not own instances.
    SchedulerStats run(
        const std::vector<LeanFFI*>& pool,
        const std::vector<Task>& tasks,
        const std::function<void(const DispatchResult&)>& on_result,
        std::atomic<bool>* cancel = nullptr);

    Policy policy() const { return policy_; }

private:
    Policy policy_;
};

}  // namespace leanffi