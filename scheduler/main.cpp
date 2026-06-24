// lpi_scheduler — drives evaluations across the instance pool.
//
// Usage:
//   lpi_scheduler --policy ROUND_ROBIN|LEAST_LOAD|DAG_AWARE \
//                 [--dispatch-all | --dispatch N] \
//                 [--repl PATH] [--target N] [--active-cap N] \
//                 [--workers W]
//
// The scheduler generates a synthetic workload: one elaboration per
// instance, with a representative tactic that exercises the kernel.
#include "instance_manager.h"
#include "scheduler.h"
#include "logger.h"
#include "evidence.h"
#include "memory_monitor.h"

#include <iostream>
#include <chrono>
#include <thread>

using namespace lpi;

static Policy parse_policy(const std::string& s) {
    if (s == "LEAST_LOAD") return Policy::LEAST_LOAD;
    if (s == "DAG_AWARE")  return Policy::DAG_AWARE;
    return Policy::ROUND_ROBIN;
}

int main(int argc, char** argv) {
    Logger::get().open("scheduler", "evolution_logs");
    InstanceManager mgr;
    std::string repl = "repl";
    size_t target = 10000;
    size_t active = 64;
    size_t workers = 8;
    std::string policy_s = "ROUND_ROBIN";
    bool do_all = false;
    long dispatch_n = -1;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--repl" && i + 1 < argc) repl = argv[++i];
        else if (a == "--target" && i + 1 < argc) target = std::stoul(argv[++i]);
        else if (a == "--active-cap" && i + 1 < argc) active = std::stoul(argv[++i]);
        else if (a == "--workers" && i + 1 < argc) workers = std::stoul(argv[++i]);
        else if (a == "--policy" && i + 1 < argc) policy_s = argv[++i];
        else if (a == "--dispatch-all") do_all = true;
        else if (a == "--dispatch" && i + 1 < argc) dispatch_n = std::stol(argv[++i]);
    }

    mgr.set_repl_path(repl);
    mgr.set_target(target);
    mgr.set_active_cap(active);
    size_t started = mgr.spawn(active);
    std::cout << "spawned=" << started << " active_cap=" << active
              << " policy=" << policy_s << "\n";

    if (started == 0) {
        std::cerr << "no instances started; aborting\n";
        return 1;
    }

    Scheduler s(mgr, parse_policy(policy_s));
    auto ids = mgr.live_ids();
    size_t n = (do_all ? ids.size() : (dispatch_n >= 0 ? (size_t)dispatch_n : ids.size()));
    // n is the *number of tasks*; we can dispatch more than ids.size()
    // because each instance services many tasks serially.

    // A small pool of well-typed kernel-friendly problems, each
    // chosen to exercise a different code path in Pantograph. The
    // cycle length is the number of problems; the rest just round-robin.
    struct Problem {
        const char* type;
        const char* tactic;
    };
    static const Problem problems[] = {
        { "True",                                "trivial" },
        { "True ∧ True",                         "constructor" },
        { "False → True",                        "intro h" },
        { "(1 : Nat) + 1 = 2",                   "rfl" },
        { "∀ (n : Nat), n = n",                  "intro n; rfl" },
        { "∀ (p q : Prop), p → q → p",           "intro p q hp hq; exact hp" },
        { "Nat",                                 "0" },
    };
    const size_t n_probs = sizeof(problems) / sizeof(problems[0]);

    for (size_t i = 0; i < n; ++i) {
        Task t;
        t.id = "t" + std::to_string(i);
        t.instance_id = ids[i % ids.size()];
        const auto& p = problems[i % n_probs];
        t.tactic = p.tactic;
        t.type   = p.type;
        t.dag = (policy_s == "DAG_AWARE" && i > 0) ? std::vector<std::string>{"t" + std::to_string(i - 1)} : std::vector<std::string>{};
        s.enqueue(t);
    }
    std::cout << "queued=" << n << " instances=" << ids.size() << "\n";

    auto t0 = std::chrono::steady_clock::now();
    size_t done = s.run(workers);
    auto t1 = std::chrono::steady_clock::now();
    double secs = std::chrono::duration<double>(t1 - t0).count();

    size_t pass = 0;
    for (auto& r : s.reports()) if (r.ok) ++pass;
    std::cout << "executed=" << done << " pass=" << pass
              << " fail=" << (done - pass)
              << " seconds=" << secs
              << " rate=" << (secs > 0 ? done / secs : 0.0) << "/s\n";
    return 0;
}
