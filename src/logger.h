#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace lpi {

// JSON-line logger with per-instance streams.
// All events go to evolution_logs/<instance_id>.jsonl AND a global
// evolution_logs/global.jsonl. The per-instance stream makes the
// filesystem-isolation invariant visible from the audit log side.
class Logger {
public:
    static Logger& get();

    void open(const std::string& instance_id, const std::string& logs_dir);

    // Event: { ts, instance, op, evidence_ref, validation, ...extras }
    void event(const std::string& instance_id,
               const std::string& op,
               const std::string& evidence_ref,
               const std::string& validation,
               const std::string& extras_json = "{}");

    void close(const std::string& instance_id);

private:
    Logger() = default;
    std::mutex mu_;
    std::ofstream global_;
};

inline std::string iso8601_now() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                  now.time_since_epoch()) % 1'000'000;
    std::tm tm{};
    gmtime_r(&t, &tm);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setw(6) << std::setfill('0') << us.count() << 'Z';
    return oss.str();
}

}  // namespace lpi
