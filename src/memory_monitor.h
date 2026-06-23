#pragma once
namespace lpi {

// Read /proc/self/status and report VmRSS in kilobytes.
// Falls back to 0 on non-Linux (we are Linux per env).
class MemoryMonitor {
public:
    static long current_rss_kb();
    static long peak_rss_kb();
};

}  // namespace lpi
