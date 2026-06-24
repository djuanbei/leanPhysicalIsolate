#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "sha256.h"

namespace leanffi {

using Bytes = std::vector<uint8_t>;

struct CmdResult {
    int exit_code = -1;
    std::string stdout_data;
    std::string stderr_data;
    bool signaled = false;
};

int exec_capture(const std::string& cmd, std::string& out, std::string& err);
CmdResult run_process(const std::string& argv0,
                     const std::vector<std::string>& args,
                     const std::vector<std::pair<std::string, std::string>>& env_extra = {},
                     const std::string& stdin_data = "");

// Filesystem
bool file_exists(const std::string& p);
bool dir_exists(const std::string& p);
bool ensure_dir(const std::string& p);
bool remove_path(const std::string& p);
bool copy_file(const std::string& src, const std::string& dst);
std::string read_file(const std::string& p);
bool write_file(const std::string& p, const std::string& data);
bool append_file(const std::string& p, const std::string& data);
std::vector<std::string> list_dir(const std::string& p);
std::string sha256_of_string(const std::string& s);
std::string sha256_of_file(const std::string& path);
std::string short_sha(const std::string& hex);

// Time
std::string now_iso8601();
uint64_t now_unix_ms();

// Strings
std::string trim(const std::string& s);
std::vector<std::string> split(const std::string& s, char sep);
std::string join(const std::vector<std::string>& v, const std::string& sep);
bool starts_with(const std::string& s, const std::string& p);
bool ends_with(const std::string& s, const std::string& p);
std::string to_lower(std::string s);

}