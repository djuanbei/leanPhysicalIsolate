#include "logger.h"
#include <iostream>

namespace lpi {

Logger& Logger::get() {
    static Logger inst;
    return inst;
}

void Logger::open(const std::string&, const std::string& logs_dir) {
    std::lock_guard<std::mutex> g(mu_);
    std::filesystem::create_directories(logs_dir);
    if (!global_.is_open()) {
        global_.open(logs_dir + "/global.jsonl", std::ios::app);
    }
}

void Logger::event(const std::string& instance_id,
                   const std::string& op,
                   const std::string& evidence_ref,
                   const std::string& validation,
                   const std::string& extras_json) {
    std::lock_guard<std::mutex> g(mu_);
    std::string line = "{\"ts\":\"" + iso8601_now()
        + "\",\"instance\":\"" + instance_id
        + "\",\"op\":\"" + op
        + "\",\"evidence_ref\":\"" + evidence_ref
        + "\",\"validation\":\"" + validation
        + "\",\"extras\":" + extras_json + "}\n";
    if (global_.is_open()) global_ << line;
    // Per-instance stream
    std::ofstream per("evolution_logs/" + instance_id + ".jsonl", std::ios::app);
    if (per.is_open()) per << line;
}

void Logger::close(const std::string&) {
    std::lock_guard<std::mutex> g(mu_);
    if (global_.is_open()) global_.flush();
}

}  // namespace lpi
