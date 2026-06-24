#include "scheduler.h"
#include "instance_manager.h"
#include "util.h"

namespace leanffi {

Scheduler::Scheduler(SchedulePolicy pol) : pol_(pol) {}

const std::string& Scheduler::policy_name() const {
    static const std::string names[3] = {
        "ROUND_ROBIN",
        "LEAST_LOAD",
        "DAG_AWARE",
    };
    return names[static_cast<int>(pol_)];
}

void Scheduler::set_pool(std::vector<std::shared_ptr<Instance>>* pool) {
    pool_ = pool;
}

void Scheduler::submit(WorkItem item) {
    std::lock_guard<std::mutex> g(mu_);
    if (item.enqueue_ms == 0) item.enqueue_ms = now_unix_ms();
    q_.push(std::move(item));
}

size_t Scheduler::pending() const {
    std::lock_guard<std::mutex> g(mu_);
    return q_.size();
}

bool Scheduler::try_assign(WorkItem& out) {
    std::lock_guard<std::mutex> g(mu_);
    if (!pool_ || q_.empty()) return false;

    WorkItem it = q_.front();
    q_.pop();

    int chosen = -1;
    if (it.instance_id >= 0) {
        // explicit assignment
        for (auto& inst : *pool_) {
            if (inst->id() == it.instance_id) { chosen = inst->id(); break; }
        }
        if (chosen < 0) chosen = -1;
    } else {
        switch (pol_) {
            case SchedulePolicy::ROUND_ROBIN: {
                if (pool_->empty()) return false;
                chosen = (*pool_)[rr_cursor_ % pool_->size()]->id();
                ++rr_cursor_;
                break;
            }
            case SchedulePolicy::LEAST_LOAD: {
                uint64_t best = UINT64_MAX;
                for (auto& inst : *pool_) {
                    uint64_t load = inst->n_evaluations.load();
                    if (load < best) { best = load; chosen = inst->id(); }
                }
                break;
            }
            case SchedulePolicy::DAG_AWARE: {
                // simple priority-aware variant: items with priority > 0 get lower load instances
                uint64_t best = UINT64_MAX;
                for (auto& inst : *pool_) {
                    uint64_t load = inst->n_evaluations.load() + (it.priority > 0 ? 0 : 1);
                    if (load < best) { best = load; chosen = inst->id(); }
                }
                break;
            }
        }
    }
    it.instance_id = chosen;
    out = std::move(it);
    assigned_.fetch_add(1);
    return true;
}

void Scheduler::release(int instance_id) {
    (void)instance_id;
    // counters already updated by instance itself
}

}