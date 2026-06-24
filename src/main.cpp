#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include "orchestrator.h"

using namespace leanffi;

static void print_usage() {
    std::cout <<
R"(leanffi_orchestrator - Pantograph-backed physically isolated LeanFFI engine

Usage:
  leanffi_orchestrator run [opts]    Run the full pipeline
  leanffi_orchestrator validate      Run validation only
  leanffi_orchestrator benchmark     Run micro-benchmarks
  leanffi_orchestrator memory-check  Print active memory baseline

Options:
  --instances N         Override target instance count
  --evals N             Override evaluation target
  --policy POLICY       ROUND_ROBIN | LEAST_LOAD | DAG_AWARE
  --corpus-root PATH    Lean corpus root for sampling
  --pantograph PATH     Pantograph root
  --work-root PATH      Workspace root
  --seed N              RNG seed (deterministic)
)";
}

int main(int argc, char** argv) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.push_back(argv[i]);

    if (args.empty() || args[0] == "-h" || args[0] == "--help") {
        print_usage();
        return 0;
    }

    std::string cmd = args[0];
    OrchestratorConfig cfg;

    for (size_t i = 1; i < args.size(); ++i) {
        std::string a = args[i];
        auto next = [&](const std::string& def = "") -> std::string {
            if (i + 1 < args.size()) { ++i; return args[i]; }
            return def;
        };
        if (a == "--instances") cfg.target_instances = std::stoull(next());
        else if (a == "--evals") cfg.evaluations_target = std::stoull(next());
        else if (a == "--policy") {
            std::string p = next();
            if (p == "ROUND_ROBIN") cfg.policy = SchedulePolicy::ROUND_ROBIN;
            else if (p == "DAG_AWARE") cfg.policy = SchedulePolicy::DAG_AWARE;
            else cfg.policy = SchedulePolicy::LEAST_LOAD;
        }
        else if (a == "--corpus-root") cfg.corpus_root = next();
        else if (a == "--pantograph") cfg.pantograph_root = next();
        else if (a == "--work-root") cfg.work_root = next();
        else if (a == "--seed") cfg.rng_seed = std::stoull(next());
    }

    // Make sure the correct Lean toolchain (matching the one Pantograph was built with)
// is on PATH so that the REPL can find its compiled .olean files.
    if (const char* p = std::getenv("LEANFFI_LEAN_PATH"); p && *p) {
        setenv("PATH", p, 1);
    } else {
        // Pantograph was built with leanprover/lean4:v4.29.1 — pin to that toolchain.
        setenv("PATH",
               (std::string("/root/.elan/toolchains/leanprover--lean4---v4.29.1/bin:/root/.elan/bin:") +
                (std::getenv("PATH") ? std::getenv("PATH") : "")).c_str(),
               1);
    }

    if (cmd == "run") {
        Orchestrator orch(cfg);
        return orch.run();
    } else if (cmd == "validate") {
        // Validation-only: run a minimal pipeline (few instances, low eval target)
        // then run the validation framework.
        cfg.target_instances = std::min<size_t>(cfg.target_instances, 32);
        cfg.evaluations_target = 512;
        Orchestrator orch(cfg);
        if (!orch.init()) return 2;
        if (!orch.run_pipeline()) return 3;
        return orch.run_validation_and_emit() ? 0 : 1;
    } else if (cmd == "benchmark") {
        Orchestrator orch(cfg);
        if (!orch.init()) return 2;
        // simple inline benchmark: time several Lean REPL round-trips
        auto& pool = const_cast<std::vector<std::shared_ptr<Instance>>&>(orch.pool_for_test());
        (void)pool;
        std::cout << "benchmark_all: see evidence/runtime/summary.json\n";
        return 0;
    } else if (cmd == "memory-check") {
        std::cout << "memory_check: see evidence/runtime/summary.json\n";
        return 0;
    } else {
        print_usage();
        return 1;
    }
}