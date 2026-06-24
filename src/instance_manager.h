#pragma once
#include <string>
#include <atomic>
#include <memory>
#include <mutex>
#include "pantograph_bridge.h"

namespace leanffi {

struct InstanceSpec {
    int id;
    std::string work_root;     // /root/mycode/lean_physical_isolate
    std::string instance_root; // /root/mycode/lean_physical_isolate/runtime/instance_<id>
    std::string env_dir;
    std::string goals_dir;
    std::string logs_dir;
    std::string cache_dir;
    std::string snapshots_dir;

    std::string repl_bin;
    std::vector<std::string> startup_args;
};

class Instance {
public:
    Instance(const InstanceSpec& spec);
    ~Instance();

    // Initialize directories and prepare isolation. Returns false on hard failure.
    bool prepare();

    // Spawn REPL process for this instance. Returns false on failure.
    bool spawn();

    // Snapshot/restore
    std::string snapshot(const std::string& tag);
    bool restore(const std::string& snapshot_id);

    // Fork: creates a new isolated instance based on the current env pickle.
    // The new instance id is returned via `new_id`.
    bool fork_into(int new_id);

    // Cleanup: remove directory tree.
    void destroy(bool keep_files = false);

    int id() const { return spec_.id; }
    const InstanceSpec& spec() const { return spec_; }
    PantographClient& client() { return *client_; }
    bool alive() const { return client_ && client_->is_alive(); }

    // stat counters
    std::atomic<uint64_t> n_evaluations{0};
    std::atomic<uint64_t> n_errors{0};
    std::atomic<uint64_t> n_snapshots{0};
    std::atomic<uint64_t> n_forks{0};

private:
    InstanceSpec spec_;
    std::unique_ptr<PantographClient> client_;
    std::mutex mu_;
};

}