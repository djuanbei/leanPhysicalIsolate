#include "leanffi.h"
#include "pantograph_protocol.h"
#include "json_helpers.h"
#include "logger.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <poll.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace lpi {

LeanFFI::LeanFFI() = default;

LeanFFI::~LeanFFI() {
    shutdown();
}

static bool read_line_nonblock(int fd, std::string& out, int timeout_ms = 10000) {
    out.clear();
    char buf[4096];
    // Read line-by-line with a deadline.
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeout_ms);
    std::string acc;
    while (true) {
        int remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                            deadline - std::chrono::steady_clock::now()).count();
        if (remaining <= 0) return !acc.empty();
        struct pollfd pfd{ fd, POLLIN, 0 };
        int pr = ::poll(&pfd, 1, remaining);
        if (pr <= 0) return !acc.empty();
        ssize_t n = ::read(fd, buf, sizeof(buf));
        if (n <= 0) return !acc.empty();
        for (ssize_t i = 0; i < n; ++i) {
            if (buf[i] == '\n') { out = acc; return true; }
            acc.push_back(buf[i]);
        }
    }
}

bool LeanFFI::init(const std::string& instance_id,
                   const std::string& workspace_root,
                   const std::string& pantograph_repl_path) {
    instance_id_ = instance_id;
    pantograph_repl_ = pantograph_repl_path;

    fs::path inst_dir = fs::path(workspace_root) / ("instance_" + instance_id);
    fs::create_directories(inst_dir / "env");
    fs::create_directories(inst_dir / "goals");
    fs::create_directories(inst_dir / "logs");
    fs::create_directories(inst_dir / "cache");
    fs::create_directories(inst_dir / "snapshots");
    workspace_ = inst_dir.string();

    int to_repl[2], from_repl[2];
    if (::pipe(to_repl) != 0) return false;
    if (::pipe(from_repl) != 0) { ::close(to_repl[0]); ::close(to_repl[1]); return false; }

    pid_t pid = ::fork();
    if (pid < 0) {
        ::close(to_repl[0]); ::close(to_repl[1]);
        ::close(from_repl[0]); ::close(from_repl[1]);
        return false;
    }
    if (pid == 0) {
        // Child
        ::dup2(to_repl[0], STDIN_FILENO);
        ::dup2(from_repl[1], STDOUT_FILENO);
        ::close(to_repl[0]); ::close(to_repl[1]);
        ::close(from_repl[0]); ::close(from_repl[1]);

        // Filesystem isolation: change to the instance directory.
        if (::chdir(workspace_.c_str()) != 0) { ::_exit(126); }
        // Make HOME/TMPDIR/LEAN_PATH point into the instance dir.
        std::string home = workspace_ + "/env";
        ::setenv("HOME", home.c_str(), 1);
        ::setenv("TMPDIR", (workspace_ + "/cache").c_str(), 1);
        ::setenv("LEAN_PATH", home.c_str(), 1);

        // exec the repl with no prelude imports (fast startup, kernel only).
        const char* argv[] = {
            "repl",
            "--printJsonPretty=false",
            "--automaticMode=false",
            nullptr
        };
        ::execv(pantograph_repl_path.c_str(), const_cast<char* const*>(argv));
        // If we reach here, exec failed.
        std::fprintf(stderr, "lpi: execv(%s) failed: %s\n",
                     pantograph_repl_path.c_str(), std::strerror(errno));
        ::_exit(127);
    }

    // Parent
    ::close(to_repl[0]);
    ::close(from_repl[1]);
    write_fd_ = to_repl[1];
    read_fd_  = from_repl[0];
    pid_ = pid;

    // Wait for "ready." handshake.
    std::string line;
    if (!read_line_nonblock(read_fd_, line, 30000) || line != "ready.") {
        Logger::get().event(instance_id_, "init", "n/a", "fail",
                            "{\"reason\":\"no ready handshake\"}");
        shutdown();
        return false;
    }

    // Probe version (the REPL responds to "version" with a single line).
    std::string req = protocol::build(protocol::cmd::kVersion, "null");
    std::string resp;
    Result r = exchange(req, resp);
    if (!r.ok) {
        Logger::get().event(instance_id_, "init.version", "n/a", "fail",
                            "{\"reason\":\"" + json::escape(r.error) + "\"}");
        shutdown();
        return false;
    }

    // Set options: kernel-friendly defaults
    std::string opts = "{\"printExprPretty\":false,\"printJsonPretty\":false,"
                       "\"automaticMode\":false,\"timeout\":5000}";
    req = protocol::build(protocol::cmd::kOptionsSet, opts);
    if (!exchange(req, resp).ok) {
        shutdown();
        return false;
    }
    started_ = true;
    Logger::get().event(instance_id_, "init", "n/a", "pass",
                        "{\"pid\":" + std::to_string(pid_) + "}");
    return true;
}

void LeanFFI::shutdown() {
    if (pid_ > 0) {
        // Try to terminate gracefully.
        if (write_fd_ >= 0) {
            const char* q = "quit\n";
            ssize_t _w = ::write(write_fd_, q, std::strlen(q));
            (void)_w;
        }
        int status = 0;
        // Give the child a moment, then SIGTERM, then SIGKILL.
        for (int i = 0; i < 20; ++i) {
            pid_t r = ::waitpid(pid_, &status, WNOHANG);
            if (r == pid_) break;
            struct timespec ts{0, 50'000'000};  // 50ms
            ::nanosleep(&ts, nullptr);
        }
        ::kill(pid_, SIGTERM);
        for (int i = 0; i < 20; ++i) {
            pid_t r = ::waitpid(pid_, &status, WNOHANG);
            if (r == pid_) break;
            struct timespec ts{0, 50'000'000};
            ::nanosleep(&ts, nullptr);
        }
        ::kill(pid_, SIGKILL);
        ::waitpid(pid_, &status, 0);
        pid_ = -1;
    }
    if (write_fd_ >= 0) { ::close(write_fd_); write_fd_ = -1; }
    if (read_fd_  >= 0) { ::close(read_fd_);  read_fd_  = -1; }
    started_ = false;
}

Result LeanFFI::exchange(const std::string& req_line, std::string& out_resp_line) {
    out_resp_line.clear();
    if (write_fd_ < 0) return Result::failure("ffi not initialized");
    ssize_t n = ::write(write_fd_, req_line.data(), req_line.size());
    if (n < 0) return Result::failure("write failed: " + std::string(std::strerror(errno)));
    n = ::write(write_fd_, "\n", 1);
    if (n < 0) return Result::failure("write nl failed: " + std::string(std::strerror(errno)));
    if (!read_line_nonblock(read_fd_, out_resp_line, 60'000)) {
        return Result::failure("timeout or EOF from repl");
    }
    if (out_resp_line.empty()) {
        return Result::failure("empty response");
    }
    // Error responses use a top-level "error" field.
    if (auto err = json::get_string(out_resp_line, "error")) {
        std::string desc;
        if (auto d = json::get_string(out_resp_line, "desc")) desc = *d;
        return Result::failure("repl error: " + *err + " " + desc,
                               { Diag{"error", desc, std::nullopt} });
    }
    return Result::success(out_resp_line);
}

Result LeanFFI::run_source(const std::string& source) {
    // We implement run_source as a one-shot "goal start" + "tactic exact?".
    // The Pantograph REPL exposes many commands; for plain term elaboration
    // we lean on "expr synthesize" (term -> its type).
    std::string payload = "{\"expr\":\"" + json::escape(source) + "\"}";
    std::string req = protocol::build(protocol::cmd::kExprSynth, payload);
    std::string resp;
    return exchange(req, resp);
}

Result LeanFFI::run_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) return Result::failure("cannot open file: " + path);
    std::stringstream ss; ss << in.rdbuf();
    return run_source(ss.str());
}

Result LeanFFI::goal_start(const std::string& expr, const std::string& type,
                           std::vector<GoalState>& out_goals) {
    std::string payload = "{\"expr\":\"" + json::escape(expr) +
                          "\",\"type\":\"" + json::escape(type) + "\"}";
    std::string req = protocol::build(protocol::cmd::kGoalStart, payload);
    std::string resp;
    Result r = exchange(req, resp);
    if (!r.ok) return r;

    GoalState g;
    g.raw_state = resp;
    if (auto sid = json::get_string(resp, "stateId")) g.state_id = *sid;
    else if (auto sid2 = json::get_string(resp, "goalId")) g.state_id = *sid2;
    if (auto sub = json::get_object(resp, "goals")) {
        // Parse sub-goal names: shallow scan for "name" fields.
        std::string s = *sub;
        size_t pos = 0;
        while ((pos = s.find("\"name\"", pos)) != std::string::npos) {
            size_t colon = s.find(':', pos + 6);
            if (colon == std::string::npos) break;
            size_t q1 = s.find('"', colon + 1);
            if (q1 == std::string::npos) break;
            size_t q2 = s.find('"', q1 + 1);
            if (q2 == std::string::npos) break;
            g.goals.push_back(s.substr(q1 + 1, q2 - q1 - 1));
            pos = q2 + 1;
        }
    } else {
        // Single-goal form: treat the returned state as one open goal.
        g.goals.push_back(g.state_id);
    }
    if (auto tgt = json::get_object(resp, "target")) g.target_expr = *tgt;
    out_goals.push_back(std::move(g));
    return r;
}

Result LeanFFI::tactic(const std::string& state_id, const std::string& tactic_src,
                       std::vector<GoalState>& out_goals) {
    std::string payload = "{\"stateId\":\"" + json::escape(state_id) +
                          "\",\"tactic\":\"" + json::escape(tactic_src) + "\"}";
    std::string req = protocol::build(protocol::cmd::kTactic, payload);
    std::string resp;
    Result r = exchange(req, resp);
    if (!r.ok) return r;

    GoalState g;
    g.raw_state = resp;
    if (auto sid = json::get_string(resp, "stateId")) g.state_id = *sid;
    if (auto sub = json::get_object(resp, "goals")) {
        std::string s = *sub;
        size_t pos = 0;
        while ((pos = s.find("\"name\"", pos)) != std::string::npos) {
            size_t colon = s.find(':', pos + 6);
            if (colon == std::string::npos) break;
            size_t q1 = s.find('"', colon + 1);
            if (q1 == std::string::npos) break;
            size_t q2 = s.find('"', q1 + 1);
            if (q2 == std::string::npos) break;
            g.goals.push_back(s.substr(q1 + 1, q2 - q1 - 1));
            pos = q2 + 1;
        }
    }
    out_goals.push_back(std::move(g));
    return r;
}

Result LeanFFI::lib_add(const std::string& name,
                        const std::string& type,
                        const std::optional<std::string>& value,
                        const std::string& kind) {
    std::string payload = "{\"name\":\"" + json::escape(name) +
                          "\",\"type\":\"" + json::escape(type) +
                          "\",\"kind\":\"" + json::escape(kind) + "\"";
    if (value) payload += ",\"value\":\"" + json::escape(*value) + "\"";
    payload += "}";
    std::string req = protocol::build(protocol::cmd::kLibAdd, payload);
    std::string resp;
    return exchange(req, resp);
}

Result LeanFFI::add_theorem(const std::string& name, const std::string& type, const std::string& proof) {
    return lib_add(name, type, proof, "theorem");
}
Result LeanFFI::add_lemma(const std::string& name, const std::string& type, const std::string& proof) {
    return lib_add(name, type, proof, "lemma");
}
Result LeanFFI::add_definition(const std::string& name, const std::string& type, const std::string& value) {
    return lib_add(name, type, value, "def");
}
Result LeanFFI::add_structure(const std::string& name, const std::string& fields_json) {
    return lib_add(name, fields_json, std::nullopt, "structure");
}
Result LeanFFI::add_class(const std::string& name, const std::string& extends, const std::string& fields_json) {
    return lib_add(name + " extends " + extends, fields_json, std::nullopt, "class");
}
Result LeanFFI::add_instance(const std::string& class_name, const std::string& term) {
    return lib_add(class_name + ".inst", term, std::nullopt, "instance");
}

Result LeanFFI::save_state(const std::string& state_id, std::string& out_blob) {
    std::string payload = "{\"stateId\":\"" + json::escape(state_id) + "\"}";
    std::string req = protocol::build(protocol::cmd::kSave, payload);
    std::string resp;
    Result r = exchange(req, resp);
    if (!r.ok) return r;
    out_blob = resp;
    return r;
}

Result LeanFFI::load_state(const std::string& blob, std::string& out_state_id) {
    std::string req = protocol::build(protocol::cmd::kLoad, blob);
    std::string resp;
    Result r = exchange(req, resp);
    if (!r.ok) return r;
    if (auto sid = json::get_string(resp, "stateId")) out_state_id = *sid;
    return r;
}

}  // namespace lpi
