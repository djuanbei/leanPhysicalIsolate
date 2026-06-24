#include "instance_manager.h"
#include "util.h"
#include "logger.h"
#include "evidence.h"
#include "json_min.h"

#include <fstream>
#include <sstream>

namespace leanffi {

Instance::Instance(const InstanceSpec& spec) : spec_(spec) {
    client_ = std::make_unique<PantographClient>();
}

Instance::~Instance() {
    destroy(true);
}

bool Instance::prepare() {
    ensure_dir(spec_.instance_root);
    ensure_dir(spec_.env_dir);
    ensure_dir(spec_.goals_dir);
    ensure_dir(spec_.logs_dir);
    ensure_dir(spec_.cache_dir);
    ensure_dir(spec_.snapshots_dir);
    // write instance_id sentinel
    std::ofstream f(spec_.instance_root + "/instance_id.txt");
    f << spec_.id << "\n";
    f.close();
    return true;
}

bool Instance::spawn() {
    ReplConfig cfg;
    cfg.repl_bin = spec_.repl_bin;
    cfg.work_dir = spec_.instance_root;
    cfg.timeout_ms = 30000;
    cfg.startup_args = spec_.startup_args;
    bool ok = client_->start(cfg);
    if (ok) {
        Logger::instance().event("instance_spawn",
                                 std::to_string(spec_.id),
                                 "spawn_repl",
                                 "",
                                 true);
    } else {
        Logger::instance().event("instance_spawn_fail",
                                 std::to_string(spec_.id),
                                 "spawn_repl",
                                 "",
                                 false);
    }
    return ok;
}

std::string Instance::snapshot(const std::string& tag) {
    std::lock_guard<std::mutex> g(mu_);
    std::string id = "snap_" + std::to_string(spec_.id) + "_" +
                     std::to_string(now_unix_ms());
    std::string subdir = spec_.snapshots_dir + "/" + id;
    ensure_dir(subdir);

    JsonValue env_desc_obj = obj();
    if (client_ && client_->is_alive()) {
        auto r = client_->env_describe();
        env_desc_obj = obj();
        set(env_desc_obj, "ok", r.ok);
        if (r.ok) set(env_desc_obj, "response", r.response);
        else set(env_desc_obj, "error", r.error_message);
    }

    JsonValue meta = obj();
    set(meta, "instance_id", spec_.id);
    set(meta, "tag", tag);
    set(meta, "timestamp", now_iso8601());
    set(meta, "snapshot_id", id);
    set(meta, "env_state", env_desc_obj);

    std::string meta_path = subdir + "/snapshot.json";
    write_file(meta_path, serialize(meta, true));

    n_snapshots.fetch_add(1);

    JsonValue evid = obj();
    set(evid, "snapshot_id", id);
    set(evid, "instance_id", spec_.id);
    set(evid, "tag", tag);
    EvidenceStore::instance().write_snapshot(id, evid);

    Logger::instance().event("snapshot_taken",
                             std::to_string(spec_.id),
                             "snapshot:" + tag,
                             "evidence/snapshot/" + id,
                             true);
    return id;
}

bool Instance::restore(const std::string& snapshot_id) {
    std::lock_guard<std::mutex> g(mu_);
    std::string subdir = spec_.snapshots_dir + "/" + snapshot_id;
    if (!dir_exists(subdir)) return false;
    std::string meta_path = subdir + "/snapshot.json";
    if (!file_exists(meta_path)) return false;
    // restoration semantics: rewrite env via frontend.process of pre-recorded source if available
    // for the simple model we re-spawn the REPL to a fresh state
    client_->shutdown();
    client_ = std::make_unique<PantographClient>();
    ReplConfig cfg;
    cfg.repl_bin = spec_.repl_bin;
    cfg.work_dir = spec_.instance_root;
    cfg.timeout_ms = 30000;
    cfg.startup_args = spec_.startup_args;
    bool ok = client_->start(cfg);
    Logger::instance().event("snapshot_restore",
                             std::to_string(spec_.id),
                             "restore:" + snapshot_id,
                             "evidence/snapshot/" + snapshot_id,
                             ok);
    return ok;
}

bool Instance::fork_into(int new_id) {
    std::lock_guard<std::mutex> g(mu_);
    std::string new_root = spec_.work_root + "/runtime/instance_" + std::to_string(new_id);
    if (dir_exists(new_root)) return false;
    ensure_dir(new_root);
    ensure_dir(new_root + "/env");
    ensure_dir(new_root + "/goals");
    ensure_dir(new_root + "/logs");
    ensure_dir(new_root + "/cache");
    ensure_dir(new_root + "/snapshots");
    std::ofstream f(new_root + "/instance_id.txt");
    f << new_id << "\n";

    Logger::instance().event("instance_fork",
                             std::to_string(spec_.id),
                             "fork:" + std::to_string(new_id),
                             "",
                             true);
    n_forks.fetch_add(1);
    return true;
}

void Instance::destroy(bool keep_files) {
    if (client_) {
        client_->shutdown();
        client_.reset();
    }
    if (!keep_files) {
        remove_path(spec_.instance_root);
    }
}

}