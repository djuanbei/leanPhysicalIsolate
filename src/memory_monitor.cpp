#include "memory_monitor.h"
#include <fstream>
#include <string>

namespace lpi {

long MemoryMonitor::current_rss_kb() {
    std::ifstream f("/proc/self/status");
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            long kb = 0;
            // Format: "VmRSS:     12345 kB"
            const char* p = line.c_str() + 6;
            while (*p == ' ' || *p == '\t') ++p;
            while (*p >= '0' && *p <= '9') {
                kb = kb * 10 + (*p - '0');
                ++p;
            }
            return kb;
        }
    }
    return 0;
}

long MemoryMonitor::peak_rss_kb() {
    std::ifstream f("/proc/self/status");
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("VmHWM:", 0) == 0) {
            long kb = 0;
            const char* p = line.c_str() + 6;
            while (*p == ' ' || *p == '\t') ++p;
            while (*p >= '0' && *p <= '9') {
                kb = kb * 10 + (*p - '0');
                ++p;
            }
            return kb;
        }
    }
    return 0;
}

}  // namespace lpi
