#pragma once
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include "instance_manager.h"
#include "scheduler.h"
#include "corpus_sampler.h"
#include "validation.h"

namespace leanffi {

struct OrchestratorConfig {
    std::string work_root = "/root/mycode/lean_physical_isolate";
    std::string pantograph_root = "/root/mycode/Pantograph";
    std::string corpus_root = "/root/mycode/lean4";
    size_t target_instances = 10000;
    size_t evaluations_target = 100000;
    uint32_t repl_timeout_ms = 15000;
    SchedulePolicy policy = SchedulePolicy::LEAST_LOAD;
    uint64_t rng_seed = 0x5eed5eedULL;
};

class Orchestrator {
public:
    explicit Orchestrator(OrchestratorConfig cfg);
    ~Orchestrator();

    int run(); // entry point: executes full pipeline

    // for tests / external driving
    bool init();
    bool run_pipeline();
    bool run_validation_and_emit();
    void shutdown_all();

    const std::vector<std::shared_ptr<Instance>>& pool_for_test() const { return pool_; }
    Scheduler& scheduler_for_test() { return sched_; }

private:
    OrchestratorConfig cfg_;
    std::vector<std::shared_ptr<Instance>> pool_;
    Scheduler sched_;
    CorpusSampler corpus_;
    ValidationFramework val_;
    std::atomic<uint64_t> total_evaluations_{0};
    std::string session_id_;
    uint64_t start_ms_ = 0;
};

}