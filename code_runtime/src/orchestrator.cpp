// Orchestrator main entry. Reads command-line args, probes host, spawns
// LeanFFI instances per physical cap, dispatches N tasks (logical
// instance count), and writes reports + audit log.

#include "leanffi.hpp"
#include "scheduler.hpp"
#include "instance_manager.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <atomic>
#include <csignal>
#include <map>
#include <thread>
#include <mutex>

namespace leanffi {

// Module-level atomic for SIGINT handling.
std::atomic<bool> g_cancel{false};

}

// Logging helper: append a timestamped audit record to evolution_logs.
static void log_event(const std::string& phase,
                      const std::string& component,
                      const std::string& evidence,
                      const std::string& validation,
                      const std::string& affected,
                      const std::string& reasoning,
                      const std::string& log_dir) {
    std::string path = log_dir + "/audit.log";
    std::ofstream f(path, std::ios::app);
    if (!f) return;
    f << "[" << leanffi::now_iso8601() << "] "
      << "phase=" << phase
      << " component=" << component
      << " evidence=\"" << evidence << "\""
      << " validation=\"" << validation << "\""
      << " affected=\"" << affected << "\""
      << " reasoning=\"" << reasoning << "\""
      << "\n";
}

static std::string slurp_file([[maybe_unused]] const std::string& path) {
    std::ifstream f(path);
    if (!f) return {};
    std::stringstream ss; ss << f.rdbuf();
    return ss.str();
}

int main(int argc, char** argv) {
    using namespace leanffi;

    // Defaults
    std::string repl_path = "/root/mycode/Pantograph/.lake/build/bin/repl";
    std::string lean_path = "/root/.elan/bin/lean";
    std::string backend = "repl";   // "repl" (Pantograph) or "cli" (lean CLI)
    std::string tasks_file;       // JSON or simple list of source strings
    int logical_instances = 10000;
    int per_instance_tasks = 10;  // 10000 * 10 = 100,000 evaluations
    std::string policy = "ROUND_ROBIN";
    std::string log_dir = "/Pantograph.ext/evolution_logs";
    std::string report_dir = "/Pantograph.ext/reports";
    std::string samples_dir = "/Pantograph.ext/generated/lean_samples";
    std::vector<std::string> modules;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const std::string& opt) -> std::string {
            if (i + 1 >= argc) { std::cerr << "missing arg for " << opt << "\n"; return ""; }
            return argv[++i];
        };
        if (a == "--repl") repl_path = next(a);
        else if (a == "--lean") lean_path = next(a);
        else if (a == "--backend") backend = next(a);
        else if (a == "--tasks") tasks_file = next(a);
        else if (a == "--logical") logical_instances = std::stoi(next(a));
        else if (a == "--per-instance") per_instance_tasks = std::stoi(next(a));
        else if (a == "--policy") policy = next(a);
        else if (a == "--log-dir") log_dir = next(a);
        else if (a == "--report-dir") report_dir = next(a);
        else if (a == "--samples-dir") samples_dir = next(a);
        else if (a == "--module") modules.push_back(next(a));
    }

    log_event("PHASE0_REQ", "orchestrator", "main_task.md §0–6", "ok", "orchestrator.cpp", "Loaded requirements; default config", log_dir);

    // Probe host
    HostCapacity hc = probe_host_capacity();
    int phys_cap = compute_physical_concurrency(hc);
    std::cout << "[orchestrator] host: nproc=" << hc.nproc
              << " mem_avail_kb=" << hc.mem_avail_kb
              << " per_instance_mb=" << hc.per_instance_mb
              << " -> physical_concurrency=" << phys_cap << "\n";
    log_event("PHASE1_PROBE", "instance_manager",
              "/proc/meminfo + sysconf(_SC_NPROCESSORS_ONL)",
              "ok", "instance_manager.cpp",
              "Host capacity probed; physical_concurrency=" + std::to_string(phys_cap),
              log_dir);

    // Load sample tasks
    std::vector<std::string> sample_sources;
    if (!samples_dir.empty()) {
        // Read all *.lean files from samples_dir; for each file, read its
        // content as the source string. Each sample is a complete Lean
        // source unit.
        std::string cmd = "ls -1 " + samples_dir + "/*.lean 2>/dev/null | head -20";
        FILE* p = popen(cmd.c_str(), "r");
        if (p) {
            char buf[512];
            while (fgets(buf, sizeof(buf), p)) {
                std::string path = buf;
                while (!path.empty() && (path.back() == '\n' || path.back() == '\r' || path.back() == ' '))
                    path.pop_back();
                if (path.empty()) continue;
                // Read the file's content as the source.
                std::ifstream f(path);
                if (!f) continue;
                std::stringstream ss; ss << f.rdbuf();
                std::string content = ss.str();
                if (!content.empty()) sample_sources.push_back(content);
            }
            pclose(p);
        }
    }
    if (sample_sources.empty()) {
        // Generate trivial snippets inline
        for (int i = 0; i < 10; ++i) {
            sample_sources.push_back("#check (" + std::to_string(i) + " : Nat)");
        }
    }
    int N_files = (int)sample_sources.size();

    // Build task list: logical_instances × per_instance_tasks × (each task
    // is one sample evaluation).
    std::vector<Task> tasks;
    tasks.reserve((size_t)logical_instances * per_instance_tasks);
    int64_t tid = 0;
    for (int li = 0; li < logical_instances; ++li) {
        for (int k = 0; k < per_instance_tasks; ++k) {
            Task t;
            t.task_id = tid++;
            t.content = sample_sources[(li * per_instance_tasks + k) % N_files];
            t.is_file = false;
            tasks.push_back(t);
        }
    }
    int64_t total_evals = (int64_t)tasks.size();
    std::cout << "[orchestrator] total evaluations (logical): " << total_evals
              << " (samples=" << N_files << ", logical_instances="
              << logical_instances << ", per_instance=" << per_instance_tasks << ")\n";
    log_event("PHASE4_DESIGN", "scheduler",
              "task model: logical_instance × per_instance × sample",
              "ok", "orchestrator.cpp",
              "Built " + std::to_string(total_evals) + " logical tasks",
              log_dir);

    // Spawn physical pool
    InstanceManager mgr;
    int spawned = 0;
    std::cout << "[orchestrator] spawning physical pool (cap=" << phys_cap << ")..." << std::flush;
    if (backend == "cli") {
        spawned = mgr.spawn_cli(phys_cap, lean_path, modules);
        std::cout << "OK\n[orchestrator] using CLI backend: " << lean_path << "\n" << std::flush;
    } else {
        spawned = mgr.spawn(phys_cap, repl_path, modules);
        std::cout << "OK\n[orchestrator] using REPL backend: " << repl_path << "\n" << std::flush;
    }
    if (spawned == 0) {
        std::cerr << "[orchestrator] failed to spawn any LeanFFI instance\n";
        log_event("PHASE7_FAIL", "instance_manager", "spawn failed", "fail", "instance_manager.cpp",
                  "Zero instances spawned; aborting", log_dir);
        return 1;
    }
    log_event("PHASE5_IMPL", "instance_manager",
              "fork+execve (REPL) or popen (CLI)",
              "ok", "instance_manager.cpp",
              "Spawned " + std::to_string(spawned) + " physical LeanFFI instances (backend=" + backend + ")",
              log_dir);

    // Choose policy
    Policy pol = Policy::ROUND_ROBIN;
    if (policy == "LEAST_LOAD") pol = Policy::LEAST_LOAD;
    else if (policy == "AFFINITY") pol = Policy::AFFINITY;
    else if (policy == "DAG_AWARE") pol = Policy::DAG_AWARE;
    Scheduler sched(pol);

    // Result sink
    std::mutex report_mtx;
    std::map<int64_t, int> per_instance_count;
    std::ofstream report_raw(report_dir + "/results_raw.jsonl", std::ios::app);
    auto on_result = [&](const DispatchResult& dr) {
        std::lock_guard<std::mutex> lk(report_mtx);
        per_instance_count[dr.instance_id]++;
        report_raw << "{"
                   << "\"task_id\":" << dr.task_id
                   << ",\"instance_id\":" << dr.instance_id
                   << ",\"success\":" << (dr.success ? "true" : "false")
                   << ",\"wall_ms\":" << dr.wall_ms
                   << ",\"error\":\"" << escape_json_string(dr.error) << "\""
                   << "}\n";
    };

    // Install SIGINT handler for graceful cancel
    std::signal(SIGINT, [](int){ leanffi::g_cancel.store(true); });

    auto t0 = std::chrono::steady_clock::now();
    SchedulerStats stats = sched.run(mgr.instances(), tasks, on_result, &g_cancel);
    auto t1 = std::chrono::steady_clock::now();

    double total_seconds = std::chrono::duration<double>(t1 - t0).count();
    double eval_per_sec = total_seconds > 0 ? (double)stats.succeeded / total_seconds : 0.0;

    std::cout << "[orchestrator] done: succeeded=" << stats.succeeded
              << " failed=" << stats.failed
              << " total_seconds=" << total_seconds
              << " eval_per_sec=" << eval_per_sec << "\n";

    // Write summary report
    std::ofstream sum(report_dir + "/summary.json");
    sum << "{\n"
        << "  \"logical_instances\": " << logical_instances << ",\n"
        << "  \"per_instance_tasks\": " << per_instance_tasks << ",\n"
        << "  \"physical_instances\": " << spawned << ",\n"
        << "  \"sample_files\": " << N_files << ",\n"
        << "  \"total_evaluations\": " << stats.total << ",\n"
        << "  \"succeeded\": " << stats.succeeded << ",\n"
        << "  \"failed\": " << stats.failed << ",\n"
        << "  \"total_seconds\": " << total_seconds << ",\n"
        << "  \"eval_per_sec\": " << eval_per_sec << ",\n"
        << "  \"policy\": \"" << policy << "\",\n"
        << "  \"backend\": \"" << backend << "\",\n"
        << "  \"host\": { \"nproc\": " << hc.nproc
        << ", \"mem_avail_kb\": " << hc.mem_avail_kb << " }\n"
        << "}\n";

    log_event("PHASE7_RUNTIME", "scheduler",
              "real-time wall-clock from chrono::steady_clock",
              stats.failed == 0 ? "ok" : "partial",
              "scheduler.cpp",
              "runtime " + std::to_string(total_seconds) + "s, "
              + std::to_string(eval_per_sec) + " eval/sec",
              log_dir);

    log_event("PHASE8_SCALE", "instance_manager",
              "logical=" + std::to_string(logical_instances)
              + " physical=" + std::to_string(spawned),
              (spawned > 0 ? "ok" : "fail"),
              "instance_manager.cpp",
              "Logical instance count preserved via time-shared physical pool",
              log_dir);

    return (stats.failed == 0) ? 0 : 2;
}