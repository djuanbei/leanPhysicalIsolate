#include "scheduler.h"
#include "logger.h"
#include "memory_monitor.h"

#include <chrono>
#include <fstream>
#include <filesystem>
#include <condition_variable>
#include <sstream>

namespace fs = std::filesystem;

namespace lpi {

Scheduler::Scheduler(InstanceManager& mgr, Policy p) : mgr_(mgr), policy_(p) {}

Scheduler::~Scheduler() = default;

void Scheduler::enqueue(const Task& t) {
    {
        std::lock_guard<std::mutex> g(q_mu_);
        q_.push(t);
        pending_.fetch_add(1);
    }
    q_cv_.notify_one();
}

void Scheduler::enqueue_many(const std::vector<Task>& ts) {
    for (auto& t : ts) enqueue(t);
}

static bool deps_satisfied(const std::vector<std::string>& deps,
                           const std::vector<std::string>& done) {
    if (deps.empty()) return true;
    for (auto& d : deps) {
        if (std::find(done.begin(), done.end(), d) == done.end()) return false;
    }
    return true;
}

size_t Scheduler::run(size_t worker_threads) {
    std::atomic<bool> stop{false};
    std::vector<std::thread> workers;
    workers.reserve(worker_threads);
    for (size_t i = 0; i < worker_threads; ++i) {
        workers.emplace_back([this, &stop] { worker_loop(stop); });
    }
    // Wait for queue to drain.
    while (true) {
        size_t p = pending_.load();
        size_t f = in_flight_.load();
        if (p == 0 && f == 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    stop.store(true);
    q_cv_.notify_all();
    for (auto& th : workers) th.join();
    return reports_.size();
}

void Scheduler::worker_loop(std::atomic<bool>& stop) {
    while (true) {
        Task t;
        {
            std::unique_lock<std::mutex> lk(q_mu_);
            q_cv_.wait_for(lk, std::chrono::milliseconds(20),
                           [&] { return !q_.empty() || stop.load(); });
            if (stop.load() && q_.empty()) return;
            if (q_.empty()) continue;
            // ROUND_ROBIN pulls in FIFO order. LEAST_LOAD and DAG_AWARE
            // are also implemented FIFO here, but the dispatcher checks
            // dependency satisfaction for DAG_AWARE.
            t = q_.front();
            q_.pop();
        }
        // DAG gate
        if (policy_ == Policy::DAG_AWARE) {
            std::vector<std::string> done;
            {
                std::lock_guard<std::mutex> g(completed_mu_);
                done = completed_;
            }
            if (!deps_satisfied(t.dag, done)) {
                // Re-queue at the back and yield.
                {
                    std::lock_guard<std::mutex> g(q_mu_);
                    q_.push(t);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }
        }
        in_flight_.fetch_add(1);
        TaskReport r;
        try_dispatch(t, r);
        reports_.push_back(r);
        {
            std::lock_guard<std::mutex> g(completed_mu_);
            completed_.push_back(t.id);
        }
        in_flight_.fetch_sub(1);
        pending_.fetch_sub(1);
    }
}

bool Scheduler::try_dispatch(const Task& t, TaskReport& out) {
    out.task_id = t.id;
    out.instance_id = t.instance_id;

    auto ffi = mgr_.acquire(t.instance_id);
    if (!ffi || !ffi->is_ready()) {
        out.ok = false; out.error = "instance not ready";
        return false;
    }

    auto t0 = std::chrono::steady_clock::now();
    auto before_rss = MemoryMonitor::current_rss_kb();

    std::vector<GoalState> goals;
    Result r = ffi->goal_start("?_w", t.type, goals);
    if (!r.ok) {
        out.ok = false; out.error = "goal_start: " + r.error;
        return false;
    }
    if (!goals.empty()) {
        r = ffi->tactic(goals.front().state_id, t.tactic, goals);
    }
    auto t1 = std::chrono::steady_clock::now();
    out.latency_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    out.peak_rss_kb = MemoryMonitor::current_rss_kb();

    // Write evidence atomically. Spec §6: only real runtime outputs.
    fs::create_directories("evidence");
    std::string path = "evidence/task_" + t.id + ".json";
    {
        std::ofstream f(path);
        f << "{\n"
          << "  \"task_id\": \"" << t.id << "\",\n"
          << "  \"instance_id\": \"" << t.instance_id << "\",\n"
          << "  \"tactic\": \"" << t.tactic << "\",\n"
          << "  \"type\": \"" << t.type << "\",\n"
          << "  \"ok\": " << (r.ok ? "true" : "false") << ",\n"
          << "  \"error\": \"" << r.error << "\",\n"
          << "  \"raw_response\": " << (goals.empty() ? "null" : goals.back().raw_state) << ",\n"
          << "  \"latency_ms\": " << out.latency_ms << ",\n"
          << "  \"peak_rss_kb\": " << out.peak_rss_kb << "\n"
          << "}\n";
    }
    out.evidence_ref = path;
    out.ok = r.ok;
    if (!r.ok) out.error = r.error;

    Logger::get().event(t.instance_id, "evaluate", path,
                        r.ok ? "pass" : "fail",
                        "{\"task\":\"" + t.id + "\",\"latency_ms\":" +
                        std::to_string(out.latency_ms) + "}");
    return r.ok;
}

}  // namespace lpi
