// Instance manager: orchestrates the lifecycle of N (logical) LeanFFI
// instances given a host capacity. On resource-constrained hosts the
// physical concurrency is bounded; logical instances > physical are
// time-shared through the scheduler.

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace leanffi {

class LeanFFI;  // forward declaration

struct HostCapacity {
    int nproc = 1;             // detected CPU count
    int64_t mem_total_kb = 0;  // /proc/meminfo MemTotal
    int64_t mem_avail_kb = 0;  // /proc/meminfo MemAvailable
    int per_instance_mb = 200; // estimated per-LeanFFI memory
    int concurrency_factor = 2; // CPU multiplier
};

// Probe host capacity from /proc and sysconf.
HostCapacity probe_host_capacity();

// Compute physical concurrency cap from host capacity.
int compute_physical_concurrency(const HostCapacity& hc);

class InstanceManager {
public:
    InstanceManager();
    ~InstanceManager();

    // Spawn up to physical_concurrency LeanFFI instances.
    // Returns number actually spawned.
    int spawn(int physical_concurrency,
              const std::string& repl_path,
              const std::vector<std::string>& modules);

    std::vector<LeanFFI*>& instances() { return pool_; }
    const std::vector<LeanFFI*>& instances() const { return pool_; }
    size_t physical_count() const { return pool_.size(); }

    // Total logical evaluations target = logical_instances (e.g. 10000)
    int64_t logical_target_ = 0;

private:
    std::vector<std::unique_ptr<LeanFFI>> owners_;
    std::vector<LeanFFI*> pool_;
};

}  // namespace leanffi