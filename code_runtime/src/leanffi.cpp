// LeanFFI implementation: spawn Pantograph REPL as a subprocess and
// drive it over its JSON-line protocol. Each instance is fully
// independent: own PID, own pipes, own Lean environment.

#include "leanffi.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace leanffi {

namespace {

// Set fd to non-blocking.
bool set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0;
}

// Wait for fd readable, with timeout.
bool wait_readable(int fd, double timeout_seconds) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    struct timeval tv;
    tv.tv_sec = (time_t)timeout_seconds;
    tv.tv_usec = (suseconds_t)((timeout_seconds - (double)tv.tv_sec) * 1e6);
    int rv = select(fd + 1, &rfds, nullptr, nullptr, &tv);
    return rv > 0;
}

// Drain available data from fd.
std::string drain_fd(int fd) {
    char buf[4096];
    std::string out;
    while (true) {
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n > 0) {
            out.append(buf, (size_t)n);
        } else if (n == 0) {
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            if (errno == EINTR) continue;
            break;
        }
    }
    return out;
}

}  // namespace

std::string now_iso8601() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    struct tm tm{};
    gmtime_r(&t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return std::string(buf);
}

std::string escape_json_string(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char esc[8];
                    std::snprintf(esc, sizeof(esc), "\\u%04x", c);
                    out += esc;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

LeanFFI::LeanFFI(int64_t instance_id, const std::string& repl_path,
                 const std::vector<std::string>& modules)
    : instance_id_(instance_id), repl_path_(repl_path), modules_(modules) {}

LeanFFI::~LeanFFI() {
    shutdown();
}

bool LeanFFI::initialize() {
    int in_pipe[2];
    int out_pipe[2];
    if (pipe(in_pipe) != 0) return false;
    if (pipe(out_pipe) != 0) {
        close(in_pipe[0]); close(in_pipe[1]);
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        return false;
    }
    if (pid == 0) {
        // Child: stdin <- in_pipe[0], stdout -> out_pipe[1], stderr -> /dev/null
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);

        // Build argv: repl_path + module args
        std::vector<std::string> args;
        args.push_back(repl_path_);
        for (const auto& m : modules_) args.push_back(m);
        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        for (auto& a : args) argv.push_back(a.data());
        argv.push_back(nullptr);

        execv(repl_path_.c_str(), argv.data());
        // If exec fails:
        _exit(127);
    }

    // Parent: close child ends
    close(in_pipe[0]);
    close(out_pipe[1]);

    pid_ = pid;
    stdin_fd_ = in_pipe[1];
    stdout_fd_ = out_pipe[0];
    set_nonblock(stdout_fd_);

    // Read until "ready." is seen, up to 60s.
    std::string buf;
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed > 60.0) {
            kill_subprocess();
            return false;
        }
        if (!wait_readable(stdout_fd_, 0.5)) continue;
        std::string chunk = drain_fd(stdout_fd_);
        buf += chunk;
        if (buf.find("ready.") != std::string::npos) return true;
    }
}

void LeanFFI::kill_subprocess() {
    if (pid_ > 0) {
        kill(pid_, SIGTERM);
        int status;
        waitpid(pid_, &status, WNOHANG);
        pid_ = -1;
    }
    if (stdin_fd_ >= 0) { close(stdin_fd_); stdin_fd_ = -1; }
    if (stdout_fd_ >= 0) { close(stdout_fd_); stdout_fd_ = -1; }
}

void LeanFFI::shutdown() {
    if (pid_ > 0) {
        // Empty line aborts the REPL loop.
        const char* empty = "\n";
        ssize_t n = write(stdin_fd_, empty, 1);
        (void)n;
        // Give it a moment to exit cleanly.
        for (int i = 0; i < 20; ++i) {
            int status;
            pid_t r = waitpid(pid_, &status, WNOHANG);
            if (r == pid_) { pid_ = -1; break; }
            usleep(50000);
        }
        kill_subprocess();
    }
}

bool LeanFFI::send_command(const std::string& cmd, const std::string& payload_json) {
    std::string line;
    if (payload_json.empty() || payload_json == "null") {
        line = cmd + "\n";
    } else {
        line = cmd + " " + payload_json + "\n";
    }
    const char* p = line.data();
    size_t remaining = line.size();
    while (remaining > 0) {
        ssize_t n = write(stdin_fd_, p, remaining);
        if (n > 0) { p += n; remaining -= (size_t)n; }
        else if (n < 0 && (errno == EINTR)) continue;
        else return false;
    }
    return true;
}

bool LeanFFI::read_response_line(std::string& out, double timeout_seconds) {
    std::string buf;
    auto start = std::chrono::steady_clock::now();
    while (true) {
        // Look for newline in any previously read data carried by drain
        size_t pos = buf.find('\n');
        if (pos != std::string::npos) {
            out = buf.substr(0, pos);
            return true;
        }
        auto elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed > timeout_seconds) {
            out = buf;
            return false;
        }
        if (!wait_readable(stdout_fd_, 0.2)) continue;
        std::string chunk = drain_fd(stdout_fd_);
        if (chunk.empty()) {
            // EAGAIN — try again
            continue;
        }
        buf += chunk;
    }
}

Result LeanFFI::execute(const Task& task) {
    Result r;
    r.instance_id = instance_id_;
    auto start = std::chrono::steady_clock::now();

    if (task.kind == SourceKind::File) {
        // Use frontend.process with fileName to leverage Pantograph's
        // exact same elaboration pipeline as `lean` CLI on a file.
        std::string payload = "{\"fileName\": \"" + escape_json_string(task.content) + "\"}";
        if (!send_command("frontend.process", payload)) {
            r.error_message = "send failed";
            return r;
        }
    } else {
        // Source string: use frontend.process with inline `file` field
        // (matches Pantograph's REPL command `frontend.process`).
        std::string payload = "{\"file\": \"" + escape_json_string(task.content) + "\"}";
        if (!send_command("frontend.process", payload)) {
            r.error_message = "send failed";
            return r;
        }
    }

    std::string resp;
    if (!read_response_line(resp, 30.0)) {
        r.error_message = "timeout or read failure";
        return r;
    }
    r.stdout_text = resp;
    // Heuristic: success if response contains "sorries" or "messages" without "error" key
    bool has_error = resp.find("\"error\"") != std::string::npos;
    if (!has_error) r.success = true;
    else r.error_message = resp;

    auto end = std::chrono::steady_clock::now();
    r.wall_seconds = std::chrono::duration<double>(end - start).count();
    return r;
}

std::vector<std::unique_ptr<LeanFFI>> spawn_instances(
    size_t n, const std::string& repl_path,
    const std::vector<std::string>& modules) {
    std::vector<std::unique_ptr<LeanFFI>> out;
    out.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        auto inst = std::make_unique<LeanFFI>((int64_t)i, repl_path, modules);
        if (!inst->initialize()) {
            // Initialization failed — skip this instance.
            continue;
        }
        out.push_back(std::move(inst));
    }
    return out;
}

}  // namespace leanffi