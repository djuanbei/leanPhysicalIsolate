// LeanFFI: thin FFI wrapper over Pantograph REPL process.
// Each LeanFFI instance is an independent OS process with its own
// Pantograph REPL subprocess. Communication is JSON-line based.

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <cstdint>

namespace leanffi {

// Result of a LeanFFI evaluation.
struct Result {
    bool success = false;
    std::string stdout_text;        // captured stdout
    std::string stderr_text;        // captured stderr
    std::string error_message;      // if !success
    double wall_seconds = 0.0;      // wall-clock time
    int64_t instance_id = -1;       // which instance produced this
};

// Source of truth: file or raw source string.
enum class SourceKind {
    File,
    Source,
};

// One task to be dispatched.
struct Task {
    int64_t task_id = 0;
    SourceKind kind = SourceKind::Source;
    bool is_file = false;           // true: kind=File expects path; false: kind=Source
    std::string content;            // file path or source code
};

// One LeanFFI instance = one Pantograph REPL subprocess + IPC.
// (or, in CLI mode, spawns a fresh `lean` per task; semantically equivalent.)
enum class LeanFFIMode {
    REPL,    // persistent Pantograph REPL subprocess
    CLI,     // spawn `lean` per task
};

class LeanFFI {
public:
    LeanFFI(int64_t instance_id, const std::string& backend_path,
            const std::vector<std::string>& modules,
            LeanFFIMode mode = LeanFFIMode::REPL);
    ~LeanFFI();

    // Initialize the REPL (waits for "ready." line). No-op in CLI mode.
    bool initialize();

    // Execute one task (file or source) on this instance.
    Result execute(const Task& task);

    // Finalize: graceful shutdown via empty line. No-op in CLI mode.
    void shutdown();

    bool alive() const { return mode_ == LeanFFIMode::CLI ? true : (pid_ > 0); }
    int64_t id() const { return instance_id_; }
    LeanFFIMode mode() const { return mode_; }

private:
    int64_t instance_id_;
    std::string backend_path_;     // REPL path or lean path
    std::vector<std::string> modules_;
    LeanFFIMode mode_;

    // REPL state
    int pid_ = -1;
    int stdin_fd_ = -1;
    int stdout_fd_ = -1;

    bool send_command(const std::string& cmd, const std::string& payload_json);
    bool read_response_line(std::string& out, double timeout_seconds);
    void kill_subprocess();
    Result execute_repl(const Task& task);
    Result execute_cli(const Task& task);
};

// Factory: spawn N independent LeanFFI instances. Returns a vector of
// (instance_id, raw pointer) for ownership at the caller.
std::vector<std::unique_ptr<LeanFFI>> spawn_instances(
    size_t n,
    const std::string& repl_path,
    const std::vector<std::string>& modules);

// Factory: spawn N CLI-mode instances (one `lean` invocation per task).
std::vector<std::unique_ptr<LeanFFI>> spawn_instances_cli(
    size_t n,
    const std::string& lean_path,
    const std::vector<std::string>& modules);

// Helpers
std::string now_iso8601();
std::string escape_json_string(const std::string& s);

}  // namespace leanffi