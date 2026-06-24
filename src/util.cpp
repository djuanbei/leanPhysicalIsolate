#include "util.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <dirent.h>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <vector>
#include <array>

namespace leanffi {

int exec_capture(const std::string& cmd, std::string& out, std::string& err) {
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return -1;
    char buf[4096];
    out.clear();
    while (true) {
        size_t n = fread(buf, 1, sizeof(buf), p);
        if (n == 0) break;
        out.append(buf, n);
    }
    int rc = pclose(p);
    return WEXITSTATUS(rc);
}

static ssize_t read_all(int fd, std::string& buf) __attribute__((unused));
static ssize_t read_all(int fd, std::string& buf) {
    char tmp[4096];
    ssize_t total = 0;
    while (true) {
        ssize_t n = read(fd, tmp, sizeof(tmp));
        if (n > 0) { buf.append(tmp, n); total += n; }
        else if (n == 0) break;
        else if (errno == EINTR) continue;
        else return -1;
    }
    return total;
}

CmdResult run_process(const std::string& argv0,
                      const std::vector<std::string>& args,
                      const std::vector<std::pair<std::string, std::string>>& env_extra,
                      const std::string& stdin_data) {
    CmdResult r;
    int in_pipe[2]; (void)in_pipe;
    int out_pipe[2];
    int err_pipe[2];
    if (pipe(out_pipe) != 0) { r.exit_code = -1; return r; }
    if (pipe(err_pipe) != 0) { close(out_pipe[0]); close(out_pipe[1]); r.exit_code = -1; return r; }

    pid_t pid = fork();
    if (pid < 0) {
        close(out_pipe[0]); close(out_pipe[1]);
        close(err_pipe[0]); close(err_pipe[1]);
        r.exit_code = -1;
        return r;
    }
    if (pid == 0) {
        // child
        close(out_pipe[0]);
        close(err_pipe[0]);
        dup2(out_pipe[1], STDOUT_FILENO);
        dup2(err_pipe[1], STDERR_FILENO);
        close(out_pipe[1]);
        close(err_pipe[1]);

        // argv
        std::vector<std::string> argv;
        argv.push_back(argv0);
        for (const auto& a : args) argv.push_back(a);
        std::vector<char*> cargv;
        cargv.reserve(argv.size() + 1);
        for (auto& s : argv) cargv.push_back(s.data());
        cargv.push_back(nullptr);

        // env
        for (const auto& [k, v] : env_extra) setenv(k.c_str(), v.c_str(), 1);

        execvp(argv0.c_str(), cargv.data());
        std::fprintf(stderr, "execvp failed: %s\n", argv0.c_str());
        _exit(127);
    }
    // parent
    close(out_pipe[1]);
    close(err_pipe[1]);

    if (!stdin_data.empty()) {
        // ignore stdin for now - parent doesn't write
    }

    // non-blocking read
    int fds[2] = { out_pipe[0], err_pipe[0] };
    std::string bufs[2];
    bool open_[2] = { true, true };
    while (open_[0] || open_[1]) {
        struct pollfd pfds[2];
        int cnt = 0;
        for (int i = 0; i < 2; ++i) if (open_[i]) {
            pfds[cnt].fd = fds[i];
            pfds[cnt].events = POLLIN;
            ++cnt;
        }
        int pr = poll(pfds, cnt, 5000);
        if (pr < 0) {
            if (errno == EINTR) continue;
            break;
        }
        for (int i = 0; i < cnt; ++i) {
            char tmp[4096];
            ssize_t n = read(pfds[i].fd, tmp, sizeof(tmp));
            if (n > 0) bufs[i].append(tmp, n);
            else if (n == 0) {
                open_[i] = false;
                close(pfds[i].fd);
            } else if (errno != EINTR) {
                open_[i] = false;
                close(pfds[i].fd);
            }
        }
    }
    close(out_pipe[0]);
    close(err_pipe[0]);
    int status = 0;
    waitpid(pid, &status, 0);
    r.stdout_data = std::move(bufs[0]);
    r.stderr_data = std::move(bufs[1]);
    if (WIFEXITED(status)) {
        r.exit_code = WEXITSTATUS(status);
        r.signaled = false;
    } else if (WIFSIGNALED(status)) {
        r.exit_code = 128 + WTERMSIG(status);
        r.signaled = true;
    }
    return r;
}

bool file_exists(const std::string& p) {
    struct stat st;
    return ::stat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}
bool dir_exists(const std::string& p) {
    struct stat st;
    return ::stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool ensure_dir(const std::string& p) {
    if (p.empty()) return false;
    if (dir_exists(p)) return true;
    size_t pos = 0;
    std::string cur;
    while (pos < p.size()) {
        size_t nxt = p.find('/', pos);
        if (nxt == std::string::npos) nxt = p.size();
        cur = p.substr(0, nxt);
        if (!cur.empty()) {
            if (::mkdir(cur.c_str(), 0755) != 0 && errno != EEXIST) return false;
        }
        pos = nxt + 1;
    }
    return true;
}

bool remove_path(const std::string& p) {
    struct stat st;
    if (::stat(p.c_str(), &st) != 0) return false;
    if (S_ISDIR(st.st_mode)) {
        DIR* d = opendir(p.c_str());
        if (!d) return false;
        struct dirent* de;
        while ((de = readdir(d))) {
            std::string name = de->d_name;
            if (name == "." || name == "..") continue;
            remove_path(p + "/" + name);
        }
        closedir(d);
        return ::rmdir(p.c_str()) == 0;
    } else {
        return ::unlink(p.c_str()) == 0;
    }
}

bool copy_file(const std::string& src, const std::string& dst) {
    std::ifstream in(src, std::ios::binary);
    if (!in) return false;
    std::ofstream out(dst, std::ios::binary);
    if (!out) return false;
    out << in.rdbuf();
    return out.good();
}

std::string read_file(const std::string& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

bool write_file(const std::string& p, const std::string& data) {
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << data;
    return out.good();
}

bool append_file(const std::string& p, const std::string& data) {
    std::ofstream out(p, std::ios::binary | std::ios::app);
    if (!out) return false;
    out << data;
    return out.good();
}

std::vector<std::string> list_dir(const std::string& p) {
    std::vector<std::string> out;
    DIR* d = opendir(p.c_str());
    if (!d) return out;
    struct dirent* de;
    while ((de = readdir(d))) {
        std::string name = de->d_name;
        if (name == "." || name == "..") continue;
        out.push_back(name);
    }
    closedir(d);
    std::sort(out.begin(), out.end());
    return out;
}

std::string sha256_of_string(const std::string& s) {
    return sha256_detail::hash(s);
}

std::string sha256_of_file(const std::string& path) {
    std::string data = read_file(path);
    return sha256_of_string(data);
}

std::string short_sha(const std::string& hex) {
    if (hex.size() > 12) return hex.substr(0, 12);
    return hex;
}

std::string now_iso8601() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
    gmtime_r(&t, &tm);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    char out[80];
    std::snprintf(out, sizeof(out), "%s.%03lldZ", buf, (long long)ms);
    return out;
}

uint64_t now_unix_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::string trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e-1]))) --e;
    return s.substr(b, e - b);
}

std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) { out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    out.push_back(cur);
    return out;
}

std::string join(const std::vector<std::string>& v, const std::string& sep) {
    std::string out;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) out += sep;
        out += v[i];
    }
    return out;
}

bool starts_with(const std::string& s, const std::string& p) {
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

bool ends_with(const std::string& s, const std::string& p) {
    return s.size() >= p.size() && s.compare(s.size() - p.size(), p.size(), p) == 0;
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

}