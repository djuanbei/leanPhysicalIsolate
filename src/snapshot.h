#pragma once
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include "leanffi.h"

namespace lpi {

// Snapshot / Restore / Fork (spec §15).
//
// We do NOT serialize the in-process Lean environment directly (that would
// require unsafe reflection into the kernel). Instead we serialise the
// *external* protocol-level state of an instance:
//   - the sequence of (cmd, payload) exchanges issued so far
//   - a side-car metadata blob (declarations, metavariable names, etc.)
// On restore, we fork the repl subprocess fresh and replay the log.
// This is the same model that `repl save` uses internally, and it gives
// us a faithful "kernel-accurate" restoration because we are *replaying*
// kernel operations, not copying memory.
struct Snapshot {
    std::string instance_id;
    std::string path;          // absolute path on disk
    std::string replay_log;    // path to the replay log
    size_t byte_size = 0;
};

class SnapshotManager {
public:
    static SnapshotManager& get();

    // Capture a snapshot of an instance into /root/mycode/lean_physical_isolate/snapshots/
    Snapshot capture(LeanFFI& ffi, const std::string& label = "");

    // Restore into a fresh instance workspace.
    bool restore(const Snapshot& snap, const std::string& new_instance_id,
                 const std::string& workspace_root,
                 const std::string& repl_path,
                 std::shared_ptr<LeanFFI>& out_ffi);

    // Fork: restore into a brand-new instance directory.
    // Returns the new instance id.
    std::string fork(const Snapshot& snap, const std::string& new_instance_id,
                     const std::string& workspace_root,
                     const std::string& repl_path,
                     std::shared_ptr<LeanFFI>& out_ffi);

    // The replay log is a JSON-line stream of { "ts":..., "req":..., "resp":... }.
    // Every LeanFFI exchange appends to it via this hook.
    void record_exchange(const std::string& instance_id,
                         const std::string& req,
                         const std::string& resp);

private:
    SnapshotManager() = default;
    std::mutex mu_;
    std::string root_ = "/root/mycode/lean_physical_isolate/snapshots";
    std::string forks_root_ = "/root/mycode/lean_physical_isolate/forks";
};

}  // namespace lpi
