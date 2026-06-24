#include "evidence.h"
#include "util.h"

namespace leanffi {

EvidenceStore& EvidenceStore::instance() {
    static EvidenceStore e;
    return e;
}

void EvidenceStore::init(const std::string& dir, const std::string& session_id) {
    base_ = dir;
    session_ = session_id;
    ensure_dir(base_);
    ensure_dir(base_ + "/test_sampling");
    ensure_dir(base_ + "/ffi_generated");
    ensure_dir(base_ + "/validation");
    ensure_dir(base_ + "/snapshot");
    ensure_dir(base_ + "/runtime");
}

std::string EvidenceStore::write(const std::string& subdir,
                                 const std::string& tag,
                                 const JsonValue& payload) {
    ensure_dir(base_ + "/" + subdir);
    std::string ts = std::to_string(now_unix_ms());
    std::string body = serialize(payload, true);
    std::string hash = short_sha(sha256_of_string(body));
    std::string name = ts + "_" + hash + (tag.empty() ? "" : ("_" + tag)) + ".json";
    std::string full = base_ + "/" + subdir + "/" + name;
    write_file(full, body);
    return "evidence/" + subdir + "/" + name;
}

std::string EvidenceStore::write_test_sampling(const std::string& file_hash,
                                               const JsonValue& payload) {
    ensure_dir(base_ + "/test_sampling");
    std::string ts = std::to_string(now_unix_ms());
    std::string name = ts + "_" + file_hash + ".json";
    std::string full = base_ + "/test_sampling/" + name;
    write_file(full, serialize(payload, true));
    return "evidence/test_sampling/" + name;
}

std::string EvidenceStore::write_ffi_generated(const std::string& file_hash,
                                               const JsonValue& payload) {
    ensure_dir(base_ + "/ffi_generated");
    std::string ts = std::to_string(now_unix_ms());
    std::string name = ts + "_" + file_hash + ".json";
    std::string full = base_ + "/ffi_generated/" + name;
    write_file(full, serialize(payload, true));
    return "evidence/ffi_generated/" + name;
}

std::string EvidenceStore::write_validation(const std::string& tag,
                                            const JsonValue& payload) {
    return write("validation", tag, payload);
}

std::string EvidenceStore::write_snapshot(const std::string& tag,
                                          const JsonValue& payload) {
    return write("snapshot", tag, payload);
}

}