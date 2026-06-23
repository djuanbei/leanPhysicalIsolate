// Scheduler implementation. Stateless per task: pick instance, run, report.
// Parallel dispatch: each worker thread drains tasks from a shared queue.

#include "scheduler.hpp"
#include "leanffi.hpp"

#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

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

    // Shared task queue
    std::queue<size_t> q;
    for (size_t i = 0; i < tasks.size(); ++i) q.push(i);
    std::mutex q_mtx;
    std::condition_variable q_cv;
    std::atomic<size_t> next_idx{0};

    // Per-instance load counters (for LEAST_LOAD)
    std::vector<int64_t> load(pool.size(), 0);
    std::mutex load_mtx;

    std::mutex on_result_mtx;
    auto wrapped_on_result = [&](const DispatchResult& dr) {
        std::lock_guard<std::mutex> lk(on_result_mtx);
        on_result(dr);
    };

    // Worker: one thread per instance, each instance processes tasks
    // assigned to it. Use atomic counter to round-robin tasks to
    // instances (this gives true parallel execution).
    std::atomic<size_t> rr_cursor{0};
    std::vector<std::thread> workers;
    workers.reserve(pool.size());

    for (size_t w = 0; w < pool.size(); ++w) {
        workers.emplace_back([&, w]() {
            while (true) {
                if (cancel && cancel->load()) break;
                size_t i;
                {
                    std::lock_guard<std::mutex> lk(q_mtx);
                    if (q.empty()) break;
                    i = q.front();
                    q.pop();
                }
                const Task& t = tasks[i];
                // Pick instance (here: each worker has its own instance).
                size_t idx = w;
                if (policy_ == Policy::ROUND_ROBIN || policy_ == Policy::DAG_AWARE) {
                    idx = rr_cursor.fetch_add(1) % pool.size();
                } else if (policy_ == Policy::LEAST_LOAD) {
                    std::lock_guard<std::mutex> lk(load_mtx);
                    size_t best = 0;
                    int64_t mn = load[0];
                    for (size_t k = 1; k < pool.size(); ++k) {
                        if (load[k] < mn) { mn = load[k]; best = k; }
                    }
                    idx = best;
                } else if (policy_ == Policy::AFFINITY) {
                    idx = (size_t)(std::hash<std::string>{}(t.content) % pool.size());
                }
                {
                    std::lock_guard<std::mutex> lk(load_mtx);
                    load[idx]++;
                }

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

                {
                    std::lock_guard<std::mutex> lk(load_mtx);
                    if (dr.success) stats.succeeded++;
                    else stats.failed++;
                    stats.total_wall_seconds += r.wall_seconds;
                }
                wrapped_on_result(dr);
            }
        });
    }

    for (auto& th : workers) th.join();

    auto t_end = std::chrono::steady_clock::now();
    double total = std::chrono::duration<double>(t_end - t_start).count();
    if (total > 0) stats.eval_per_sec = (double)stats.succeeded / total;
    return stats;
}

}  // namespace leanffi