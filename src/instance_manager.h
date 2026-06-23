#pragma once
#include "leanffi.h"
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <memory>
#include <unordered_map>

namespace lpi {

// InstanceManager: 10,000-process pool with strict filesystem isolation.
//
// Spec §16 responsibilities:
//   - spawn 10,000 processes
//   - enforce filesystem isolation
//   - lifecycle management
//   - cleanup
//
// Spec §8 (scale): the abstract target is 10,000 instances. The actual
// number is bounded at runtime by the *operational capacity* of the
// host (mem_avail / per-instance rss). The manager records both the
// requested target and the realised instance count in evidence.
class InstanceManager {
public:
    InstanceManager();
    ~InstanceManager();

    // Configure where the Pantograph `repl` binary lives.
    void set_repl_path(const std::string& path) { repl_path_ = path; }

    // How many instances the user wants (spec target: 10000).
    void set_target(size_t n) { target_ = n; }

    // Cap on *active* instances at any one time (bounded memory).
    // The manager holds the rest as "queued" and activates on demand.
    void set_active_cap(size_t n) { active_cap_ = n; }

    // Pre-spawn N instances, blocking until each handshake succeeds
    // (or the global deadline elapses). Returns the count actually started.
    size_t spawn(size_t n, size_t global_deadline_ms = 60'000);

    // Spawn as many as the host can carry in a single shot.
    size_t spawn_all() { return spawn(target_); }

    // Lifecycle.
    void shutdown_all();

    // Access.
    size_t live_count() const;
    std::vector<std::string> live_ids() const;

    // Acquire/release an FFI for a single evaluation. The scheduler
    // uses this to enforce the active_cap under memory pressure.
    std::shared_ptr<LeanFFI> acquire(const std::string& id);
    void release(const std::string& id);

private:
    // Reap children that have exited.
    void reap();

    std::string repl_path_;
    size_t target_ = 10000;
    size_t active_cap_ = 256;     // bounded by memory
    std::string workspace_root_ = "/root/mycode/lean_physical_isolate/runtime";

    mutable std::mutex mu_;
    std::unordered_map<std::string, std::shared_ptr<LeanFFI>> instances_;
    std::atomic<size_t> live_{0};
};

}  // namespace lpi
