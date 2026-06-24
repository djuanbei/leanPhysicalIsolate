#pragma once
#include <string>
#include <mutex>
#include "json_min.h"

namespace leanffi {

class Logger {
public:
    static Logger& instance();
    void init(const std::string& log_dir, const std::string& session_id);

    // emit a structured event into the JSONL log
    void event(const std::string& event_type,
               const std::string& instance_id,
               const std::string& operation,
               const std::string& evidence_ref,
               bool validation_result,
               const JsonValue& extra = JsonValue(JsonObject{}));

    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);

    const std::string& session_id() const { return session_; }
    const std::string& log_file() const { return log_file_; }

private:
    Logger() = default;
    std::mutex mu_;
    std::string dir_;
    std::string session_;
    std::string log_file_;
};

}