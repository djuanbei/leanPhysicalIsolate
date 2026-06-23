// Instance manager implementation.

#include "instance_manager.hpp"
#include "leanffi.hpp"

#include <unistd.h>
#include <sys/sysinfo.h>
#include <fstream>
#include <sstream>
#include <string>

namespace leanffi {

HostCapacity probe_host_capacity() {
    HostCapacity hc;
    hc.nproc = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (hc.nproc < 1) hc.nproc = 1;

    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        hc.mem_total_kb = (int64_t)si.totalram * si.mem_unit / 1024;
        // Available memory approximation: free + buffers + cached
        int64_t free_kb = (int64_t)si.freeram * si.mem_unit / 1024;
        int64_t buf_kb = (int64_t)si.bufferram * si.mem_unit / 1024;
        hc.mem_avail_kb = free_kb + buf_kb;
    }
    // Try to read /proc/meminfo for more accurate MemAvailable.
    std::ifstream f("/proc/meminfo");
    std::string key, val, unit;
    while (f >> key >> val >> unit) {
        if (key == "MemAvailable:") {
            hc.mem_avail_kb = std::stoll(val);
        } else if (key == "MemTotal:") {
            hc.mem_total_kb = std::stoll(val);
        }
    }
    return hc;
}

int compute_physical_concurrency(const HostCapacity& hc) {
    // cap1: CPU-based
    int cpu_cap = hc.nproc * hc.concurrency_factor;
    // cap2: memory-based: reserve 256MB for OS+orchestrator
    int64_t usable_kb = hc.mem_avail_kb - 256 * 1024;
    if (usable_kb < 0) usable_kb = 0;
    int mem_cap = (int)(usable_kb / (hc.per_instance_mb * 1024));
    if (mem_cap < 1) mem_cap = 1;
    int cap = std::min(cpu_cap, mem_cap);
    if (cap < 1) cap = 1;
    return cap;
}

InstanceManager::InstanceManager() = default;
InstanceManager::~InstanceManager() = default;

int InstanceManager::spawn(int physical_concurrency,
                           const std::string& repl_path,
                           const std::vector<std::string>& modules) {
    auto instances = spawn_instances((size_t)physical_concurrency, repl_path, modules);
    owners_.reserve(instances.size());
    pool_.reserve(instances.size());
    for (auto& u : instances) {
        LeanFFI* p = u.get();
        pool_.push_back(p);
        owners_.push_back(std::move(u));
    }
    return (int)pool_.size();
}

int InstanceManager::spawn_cli(int physical_concurrency,
                              const std::string& lean_path,
                              const std::vector<std::string>& modules) {
    auto instances = spawn_instances_cli((size_t)physical_concurrency, lean_path, modules);
    owners_.reserve(instances.size());
    pool_.reserve(instances.size());
    for (auto& u : instances) {
        LeanFFI* p = u.get();
        pool_.push_back(p);
        owners_.push_back(std::move(u));
    }
    return (int)pool_.size();
}

}  // namespace leanffi