#include "logger.h"
#include "util.h"
#include <iostream>

namespace leanffi {

Logger& Logger::instance() {
    static Logger l;
    return l;
}

void Logger::init(const std::string& log_dir, const std::string& session_id) {
    std::lock_guard<std::mutex> g(mu_);
    dir_ = log_dir;
    session_ = session_id;
    ensure_dir(dir_);
    log_file_ = dir_ + "/events_" + session_id + ".jsonl";
    // truncate
    write_file(log_file_, "");
}

void Logger::event(const std::string& event_type,
                   const std::string& instance_id,
                   const std::string& operation,
                   const std::string& evidence_ref,
                   bool validation_result,
                   const JsonValue& extra) {
    JsonValue ev = obj();
    set(ev, "timestamp", now_iso8601());
    set(ev, "session_id", session_);
    set(ev, "event_type", event_type);
    set(ev, "instance_id", instance_id);
    set(ev, "operation", operation);
    set(ev, "evidence_ref", evidence_ref);
    set(ev, "validation_result", validation_result);
    set(ev, "extra", extra);
    std::string line = serialize(ev) + "\n";
    {
        std::lock_guard<std::mutex> g(mu_);
        if (!log_file_.empty()) {
            append_file(log_file_, line);
        }
    }
}

void Logger::info(const std::string& msg) {
    std::cerr << "[INFO] " << msg << std::endl;
}
void Logger::warn(const std::string& msg) {
    std::cerr << "[WARN] " << msg << std::endl;
}
void Logger::error(const std::string& msg) {
    std::cerr << "[ERROR] " << msg << std::endl;
}

}