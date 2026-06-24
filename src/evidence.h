#pragma once
#include <string>
#include "json_min.h"

namespace leanffi {

class EvidenceStore {
public:
    static EvidenceStore& instance();

    void init(const std::string& dir, const std::string& session_id);

    // writes a JSON file under evidence/<subdir>/<timestamp>_<hash>.json
    // returns the relative reference path
    std::string write(const std::string& subdir,
                      const std::string& tag,
                      const JsonValue& payload);

    // writes under evidence/test_sampling/
    std::string write_test_sampling(const std::string& file_hash,
                                    const JsonValue& payload);

    // writes under evidence/ffi_generated/
    std::string write_ffi_generated(const std::string& file_hash,
                                    const JsonValue& payload);

    // writes under evidence/validation/
    std::string write_validation(const std::string& tag,
                                 const JsonValue& payload);

    // writes under evidence/snapshot/
    std::string write_snapshot(const std::string& tag,
                               const JsonValue& payload);

    const std::string& base_dir() const { return base_; }

private:
    EvidenceStore() = default;
    std::string base_;
    std::string session_;
};

}