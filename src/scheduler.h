#pragma once
#include "instance_manager.h"
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <algorithm>

namespace lpi {

// Scheduler policies (spec §17).
enum class Policy {
    ROUND_ROBIN,
    LEAST_LOAD,
    DAG_AWARE,
};

// One unit of work for the scheduler. The "dag" field is a comma-separated
// list of predecessor task ids (empty = root). DAG_AWARE only dispatches
// a task when all its predecessors are done.
struct Task {
    std::string id;
    std::string instance_id;
    std::string tactic;     // source-level expression
    std::string type;       // expected type (for goal_start)
    std::vector<std::string> dag;
};

// Outcome of a dispatched task. Written to evidence/.
struct TaskReport {
    std::string task_id;
    std::string instance_id;
    bool ok = false;
    std::string error;
    double latency_ms = 0.0;
    size_t peak_rss_kb = 0;
    std::string evidence_ref;   // path under evidence/
};

class Scheduler {
public:
    Scheduler(InstanceManager& mgr, Policy p = Policy::ROUND_ROBIN);
    ~Scheduler();

    void enqueue(const Task& t);
    void enqueue_many(const std::vector<Task>& ts);

    // Drive the queue until empty, with `worker_threads` workers.
    // Returns the number of tasks executed.
    size_t run(size_t worker_threads);

    // Read-only access to the report stream.
    const std::vector<TaskReport>& reports() const { return reports_; }

private:
    void worker_loop(std::atomic<bool>& stop);
    bool try_dispatch(const Task& t, TaskReport& out);

    InstanceManager& mgr_;
    Policy policy_;
    std::mutex q_mu_;
    std::condition_variable q_cv_;
    std::queue<Task> q_;
    std::atomic<size_t> pending_{0};
    std::atomic<size_t> in_flight_{0};
    std::vector<std::string> completed_;
    std::mutex completed_mu_;

    std::vector<TaskReport> reports_;
};

}  // namespace lpi
