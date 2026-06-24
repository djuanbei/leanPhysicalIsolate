#include "pantograph_bridge.h"
#include "util.h"
#include "logger.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>
#include <cstring>
#include <cstdio>
#include <array>
#include <sstream>
#include <cstdlib>

namespace leanffi {

PantographClient::PantographClient() = default;

PantographClient::~PantographClient() {
    shutdown();
}

static bool set_nonblock(int fd) {
    int fl = fcntl(fd, F_GETFL, 0);
    if (fl < 0) return false;
    return fcntl(fd, F_SETFL, fl | O_NONBLOCK) == 0;
}

bool PantographClient::start(const ReplConfig& cfg) {
    std::lock_guard<std::mutex> g(mu_);
    cfg_ = cfg;

    int in_pipe[2];
    int out_pipe[2];
    if (pipe(in_pipe) != 0) return false;
    if (pipe(out_pipe) != 0) { close(in_pipe[0]); close(in_pipe[1]); return false; }

    pid_t pid = fork();
    if (pid < 0) {
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        return false;
    }
    if (pid == 0) {
        // child
        dup2(in_pipe[0], STDIN_FILENO);
        close(in_pipe[0]); close(in_pipe[1]);
        dup2(out_pipe[1], STDOUT_FILENO);
        close(out_pipe[0]); close(out_pipe[1]);

        if (!cfg.work_dir.empty()) {
            if (chdir(cfg.work_dir.c_str()) != 0) {
                std::fprintf(stderr, "chdir failed: %s\n", cfg.work_dir.c_str());
                _exit(127);
            }
        }

        // Redirect REPL stderr to a log file so that crashes don't pollute the parent stderr.
        if (!cfg.work_dir.empty()) {
            std::string err_log = cfg.work_dir + "/repl.stderr";
            int efd = ::open(err_log.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (efd >= 0) {
                dup2(efd, STDERR_FILENO);
                ::close(efd);
            }
        }

        std::vector<std::string> argv;
        argv.push_back(cfg.repl_bin);
        for (const auto& a : cfg.startup_args) argv.push_back(a);
        std::vector<char*> cargv;
        for (auto& s : argv) cargv.push_back(s.data());
        cargv.push_back(nullptr);

        // The Lean REPL flushes line-by-line via IO.putStr / IO.flush
        // (see Pantograph/Main.lean "printImmediate"), but stdout block-buffers
        // when not attached to a TTY. Use stdbuf wrapper to force line-buffered
        // stdout/stderr at the libc level.
        std::vector<std::string> argv2 = { "stdbuf", "-oL", "-eL", cfg.repl_bin };
        for (const auto& a : cfg.startup_args) argv2.push_back(a);
        std::vector<char*> cargv2;
        for (auto& s : argv2) cargv2.push_back(s.data());
        cargv2.push_back(nullptr);
        if (execvp("stdbuf", cargv2.data()) == -1) {
            // Fallback: try direct exec
            execvp(cfg.repl_bin.c_str(), cargv.data());
            std::fprintf(stderr, "execvp failed: %s\n", cfg.repl_bin.c_str());
            _exit(127);
        }
    }
    // parent
    close(in_pipe[0]);
    close(out_pipe[1]);
    in_fd_ = in_pipe[1];
    out_fd_ = out_pipe[0];
    pid_ = pid;

    // wait for "ready." sentinel
    std::string line;
    if (!read_one_line_blocking(line, 15000)) {
        Logger::instance().error("pantograph: did not produce 'ready.' sentinel");
        shutdown();
        return false;
    }
    if (line.find("ready.") == std::string::npos) {
        Logger::instance().warn("pantograph: unexpected first line: " + line);
    }
    return true;
}

void PantographClient::shutdown() {
    std::lock_guard<std::mutex> g(mu_);
    if (pid_ > 0) {
        // best-effort: write empty newline then close stdin to terminate the loop
        if (in_fd_ >= 0) {
            const char* nl = "\n";
            ssize_t ign = ::write(in_fd_, nl, 1); (void)ign;
        }
        // send SIGTERM if still alive
        ::kill(pid_, SIGTERM);
        int status = 0;
        for (int i = 0; i < 50; ++i) {
            pid_t r = ::waitpid(pid_, &status, WNOHANG);
            if (r == pid_) break;
            usleep(50000);
        }
        ::kill(pid_, SIGKILL);
        ::waitpid(pid_, &status, 0);
        pid_ = -1;
    }
    if (in_fd_ >= 0) { ::close(in_fd_); in_fd_ = -1; }
    if (out_fd_ >= 0) { ::close(out_fd_); out_fd_ = -1; }
}

bool PantographClient::read_one_line_blocking(std::string& out, uint32_t timeout_ms) {
    out.clear();
    if (out_fd_ < 0) return false;
    uint64_t start = now_unix_ms();
    std::string buf;
    while (true) {
        if (now_unix_ms() - start > timeout_ms) return false;
        char tmp[4096];
        ssize_t n = ::read(out_fd_, tmp, sizeof(tmp));
        if (n > 0) {
            buf.append(tmp, n);
            auto nl = buf.find('\n');
            if (nl != std::string::npos) {
                out = buf.substr(0, nl);
                return true;
            }
        } else if (n == 0) {
            return false;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                usleep(20000);
                continue;
            }
            return false;
        }
    }
}

ReplCallResult PantographClient::call(const std::string& cmd, const JsonValue& payload) {
    ReplCallResult r;
    if (pid_ <= 0 || in_fd_ < 0) {
        r.error_message = "process not running";
        return r;
    }
    uint64_t seq = seqnum_.fetch_add(1);
    JsonValue envelope = obj();
    set(envelope, "cmd", cmd);
    set(envelope, "payload", payload);
    set(envelope, "seqnum", (long long)seq);
    std::string line = serialize(envelope) + "\n";

    std::lock_guard<std::mutex> g(mu_);
    uint64_t t0 = now_unix_ms();
    const char* p = line.data();
    size_t left = line.size();
    while (left > 0) {
        ssize_t n = ::write(in_fd_, p, left);
        if (n > 0) { p += n; left -= n; }
        else if (n < 0 && (errno == EINTR)) continue;
        else break;
    }
    std::string resp;
    if (!read_one_line_blocking(resp, cfg_.timeout_ms)) {
        r.error_message = "read timeout or EOF";
        r.duration_ms = now_unix_ms() - t0;
        // detect crash: try non-blocking waitpid
        int status = 0;
        pid_t r2 = ::waitpid(pid_, &status, WNOHANG);
        if (r2 == pid_) {
            Logger::instance().warn("pantograph: subprocess died during call");
        }
        return r;
    }
    r.duration_ms = now_unix_ms() - t0;
    r.raw_line = resp;
    auto parsed = parse(resp);
    if (!parsed) {
        r.error_message = "json parse error: " + resp.substr(0, 200);
        return r;
    }
    r.response = *parsed;
    if (r.response.is_object() && r.response.contains("error")) {
        // Pantograph InteractionError
        r.ok = false;
        if (r.response.at("error").is_string()) {
            r.error_message = r.response.at("error").as_string() + ": " +
                              (r.response.contains("desc") ? r.response.at("desc").as_string() : "");
        }
        return r;
    }
    r.ok = true;
    return r;
}

ReplCallResult PantographClient::options_print() {
    return call("options.print", JsonValue(JsonObject{}));
}
ReplCallResult PantographClient::options_set(int timeout) {
    JsonValue p = obj();
    set(p, "timeout", (long long)timeout);
    return call("options.set", p);
}
ReplCallResult PantographClient::env_describe() {
    return call("env.describe", JsonValue(JsonObject{}));
}

ReplCallResult PantographClient::env_add(const std::string& name,
                                        const std::string& value,
                                        bool is_theorem,
                                        const std::string& type,
                                        const std::vector<std::string>& levels) {
    JsonValue p = obj();
    set(p, "name", name);
    if (!levels.empty()) {
        JsonValue a = arr();
        for (auto& l : levels) set_arr(a, l);
        set(p, "levels", a);
    }
    if (!type.empty()) set(p, "type", type);
    set(p, "value", value);
    set(p, "isTheorem", is_theorem);
    return call("env.add", p);
}

ReplCallResult PantographClient::expr_echo(const std::string& expr) {
    JsonValue p = obj();
    set(p, "expr", expr);
    return call("expr.echo", p);
}

ReplCallResult PantographClient::goal_start(const std::string& expr) {
    JsonValue p = obj();
    set(p, "expr", expr);
    return call("goal.start", p);
}

ReplCallResult PantographClient::goal_tactic(int state_id, const std::string& tactic) {
    JsonValue p = obj();
    set(p, "stateId", (long long)state_id);
    set(p, "tactic", tactic);
    return call("goal.tactic", p);
}

ReplCallResult PantographClient::goal_print(int state_id) {
    JsonValue p = obj();
    set(p, "stateId", (long long)state_id);
    return call("goal.print", p);
}

ReplCallResult PantographClient::goal_delete(int state_id) {
    JsonValue p = obj();
    set(p, "stateId", (long long)state_id);
    return call("goal.delete", p);
}

ReplCallResult PantographClient::reset() {
    return call("reset", JsonValue(JsonObject{}));
}

ReplCallResult PantographClient::frontend_process(const std::string& source,
                                                  const std::string& file_name,
                                                  bool read_header) {
    JsonValue p = obj();
    set(p, "file", source);
    set(p, "fileName", file_name);
    set(p, "readHeader", read_header);
    set(p, "inheritEnv", false);
    set(p, "newConstants", false);
    return call("frontend.process", p);
}

}