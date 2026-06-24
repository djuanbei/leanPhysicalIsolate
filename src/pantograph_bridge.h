#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <optional>
#include <atomic>
#include "json_min.h"

namespace leanffi {

// LeanFFI client - thin orchestration layer over the Pantograph REPL.
// LeanFFI does NOT reimplement any kernel / elaborator / tactic logic.
// All semantic operations are delegated to Pantograph via its JSON protocol.
//
// LeanFFI(x) = Compose(Pantograph(x))

struct ReplConfig {
    std::string repl_bin;        // path to Pantograph repl
    std::string work_dir;        // instance dir
    uint32_t timeout_ms = 60000; // per-call timeout
    std::vector<std::string> startup_args;
};

struct ReplCallResult {
    bool ok = false;
    JsonValue response;
    std::string raw_line;
    uint64_t duration_ms = 0;
    std::string error_message;
};

class PantographClient {
public:
    PantographClient();
    ~PantographClient();

    // Spawn REPL subprocess bound to `work_dir`. Returns false on failure.
    bool start(const ReplConfig& cfg);
    bool is_alive() const { return pid_ > 0; }
    pid_t pid() const { return pid_; }

    // Single REPL call. Sends one command line and reads one response line.
    ReplCallResult call(const std::string& cmd, const JsonValue& payload);

    // Convenience typed helpers (build correct payload).
    ReplCallResult options_print();
    ReplCallResult options_set(int timeout);
    ReplCallResult env_describe();
    ReplCallResult env_add(const std::string& name,
                           const std::string& value,
                           bool is_theorem,
                           const std::string& type = "",
                           const std::vector<std::string>& levels = {});
    ReplCallResult expr_echo(const std::string& expr);
    ReplCallResult goal_start(const std::string& expr);
    ReplCallResult goal_tactic(int state_id, const std::string& tactic);
    ReplCallResult goal_print(int state_id);
    ReplCallResult goal_delete(int state_id);
    ReplCallResult reset();
    ReplCallResult frontend_process(const std::string& source,
                                   const std::string& file_name,
                                   bool read_header = true);

    // graceful shutdown
    void shutdown();

private:
    pid_t pid_ = -1;
    int in_fd_ = -1;
    int out_fd_ = -1;
    ReplConfig cfg_;
    std::mutex mu_;
    std::atomic<uint64_t> seqnum_{0};

    bool read_one_line_blocking(std::string& out, uint32_t timeout_ms);
};

}