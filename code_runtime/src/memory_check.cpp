// memory_check: spawn a small LeanFFI pool and sample RSS over a
// fixed interval. Reports max RSS growth relative to baseline.

#include "leanffi.hpp"
#include "instance_manager.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>

namespace {

int64_t read_self_rss_kb() {
    std::ifstream f("/proc/self/status");
    std::string key, val, unit;
    while (f >> key >> val >> unit) {
        if (key == "VmRSS:") return std::stoll(val);
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    using namespace leanffi;
    std::string repl_path = "/root/mycode/Pantograph/.lake/build/bin/repl";
    int duration_sec = 30;
    int sample_interval_ms = 500;

    for (int i = 1; i + 1 < argc; ++i) {
        std::string a = argv[i];
        if (a == "--repl") repl_path = argv[++i];
        else if (a == "--duration") duration_sec = std::stoi(argv[++i]);
        else if (a == "--interval") sample_interval_ms = std::stoi(argv[++i]);
    }

    HostCapacity hc = probe_host_capacity();
    int phys_cap = std::min(compute_physical_concurrency(hc), 2);
    std::cout << "[memory_check] phys_cap=" << phys_cap
              << " duration=" << duration_sec << "s\n";

    int64_t rss0 = read_self_rss_kb();
    auto instances = spawn_instances((size_t)phys_cap, repl_path, {});
    int64_t rss1 = read_self_rss_kb();

    std::ofstream out("/Pantograph.ext/reports/memory_check.jsonl");
    int64_t rss_max = rss1;
    auto t_end = std::chrono::steady_clock::now() +
                 std::chrono::seconds(duration_sec);
    int64_t total_samples = 0;
    while (std::chrono::steady_clock::now() < t_end) {
        int64_t r = read_self_rss_kb();
        if (r > rss_max) rss_max = r;
        out << "{\"t_ms\":" << total_samples * sample_interval_ms
            << ",\"vm_rss_kb\":" << r << "}\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(sample_interval_ms));
        total_samples++;
    }

    std::ofstream sum("/Pantograph.ext/reports/memory_check_summary.json");
    sum << "{\n"
        << "  \"rss_baseline_kb\": " << rss0 << ",\n"
        << "  \"rss_after_spawn_kb\": " << rss1 << ",\n"
        << "  \"rss_max_kb\": " << rss_max << ",\n"
        << "  \"rss_growth_kb\": " << (rss_max - rss0) << ",\n"
        << "  \"instances\": " << phys_cap << ",\n"
        << "  \"duration_sec\": " << duration_sec << ",\n"
        << "  \"samples\": " << total_samples << "\n"
        << "}\n";

    std::cout << "[memory_check] rss0=" << rss0 << "kB rss_max=" << rss_max
              << "kB growth=" << (rss_max - rss0) << "kB\n";
    return 0;
}