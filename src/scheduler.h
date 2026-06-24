#pragma once
#include <vector>
#include <memory>
#include <queue>
#include <mutex>
#include <atomic>
#include <string>
#include <functional>

namespace leanffi {

class Instance;

enum class SchedulePolicy {
    ROUND_ROBIN,
    LEAST_LOAD,
    DAG_AWARE,
};

struct WorkItem {
    std::string id;
    int instance_id = -1; // -1 = unassigned
    std::string op_type;  // "run_file" | "run_source" | "add_theorem" | "add_lemma" | "tactic"
    std::string source;
    std::string tag;
    int priority = 0;
    uint64_t enqueue_ms = 0;
    std::string depends_on;
};

class Scheduler {
public:
    Scheduler(SchedulePolicy pol);

    void set_pool(std::vector<std::shared_ptr<Instance>>* pool);

    // Submit work item
    void submit(WorkItem item);
    // Get next item + assign instance (returns false if empty)
    bool try_assign(WorkItem& out);
    // Mark instance free
    void release(int instance_id);

    size_t pending() const;
    SchedulePolicy policy() const { return pol_; }
    const std::string& policy_name() const;

    uint64_t total_assigned() const { return assigned_.load(); }

private:
    SchedulePolicy pol_;
    std::vector<std::shared_ptr<Instance>>* pool_ = nullptr;
    mutable std::mutex mu_;
    std::queue<WorkItem> q_;
    size_t rr_cursor_ = 0;
    std::atomic<uint64_t> assigned_{0};
};

}