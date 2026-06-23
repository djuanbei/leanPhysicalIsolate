#pragma once
#include <cstdlib>
#include <string>

// Silences -Wunused-result on ::system() and similar POSIX helpers.
namespace lpi {
namespace sys {
inline int run(const char* cmd) { return ::system(cmd); }
inline int run(const std::string& cmd) { return ::system(cmd.c_str()); }
}  // namespace sys
}  // namespace lpi
