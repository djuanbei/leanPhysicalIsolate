// Scheduler implementation. Stateless per task: pick instance, run, report.

#include "scheduler.hpp"
#include "leanffi.hpp"

#include <chrono>
#include <atomic>

namespace leanffi {

SchedulerStats Scheduler::run(
    const std::vector<LeanFFI*>& pool,
    const std::vector<Task>& tasks,
    const std::function<void(const DispatchResult&)>& on_result,
    std::atomic<bool>* cancel) {

    SchedulerStats stats{};
    stats.total = (int64_t)tasks.size();

    if (pool.empty()) {
        // Mark all as failed
        for (const auto& t : tasks) {
            DispatchResult dr;
            dr.task_id = t.task_id;
            dr.success = false;
            dr.error = "no instances available";
            on_result(dr);
            stats.failed++;
        }
        return stats;
    }

    auto t_start = std::chrono::steady_clock::now();

    // Each instance has a "load" counter; LEAST_LOAD picks minimum.
    std::vector<int64_t> load(pool.size(), 0);

    size_t rr_cursor = 0;
    for (size_t i = 0; i < tasks.size(); ++i) {
        if (cancel && cancel->load()) break;
        const Task& t = tasks[i];

        size_t idx = 0;
        switch (policy_) {
            case Policy::ROUND_ROBIN:
                idx = rr_cursor % pool.size();
                rr_cursor++;
                break;
            case Policy::LEAST_LOAD: {
                idx = 0;
                int64_t mn = load[0];
                for (size_t k = 1; k < pool.size(); ++k) {
                    if (load[k] < mn) { mn = load[k]; idx = k; }
                }
                break;
            }
            case Policy::AFFINITY:
                // Pin by hash of content
                idx = (size_t)(std::hash<std::string>{}(t.content) % pool.size());
                break;
            case Policy::DAG_AWARE:
                // Without explicit DAG edges, fall back to round-robin.
                idx = rr_cursor % pool.size();
                rr_cursor++;
                break;
        }
        load[idx]++;

        LeanFFI* inst = pool[idx];
        Task ft;
        ft.task_id = t.task_id;
        ft.content = t.content;
        ft.kind = t.is_file ? SourceKind::File : SourceKind::Source;

        Result r = inst->execute(ft);

        DispatchResult dr;
        dr.task_id = t.task_id;
        dr.instance_id = r.instance_id;
        dr.success = r.success;
        dr.output = r.stdout_text;
        dr.error = r.error_message;
        dr.wall_seconds = r.wall_seconds;
        dr.wall_ms = (int64_t)(r.wall_seconds * 1000.0);

        if (dr.success) stats.succeeded++;
        else stats.failed++;
        stats.total_wall_seconds += r.wall_seconds;
        on_result(dr);
    }

    auto t_end = std::chrono::steady_clock::now();
    double total = std::chrono::duration<double>(t_end - t_start).count();
    if (total > 0) stats.eval_per_sec = (double)stats.succeeded / total;
    return stats;
}

}  // namespace leanffi