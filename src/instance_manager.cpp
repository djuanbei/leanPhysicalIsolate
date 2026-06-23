#include "instance_manager.h"
#include "logger.h"

#include <filesystem>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

namespace lpi {

InstanceManager::InstanceManager() = default;

InstanceManager::~InstanceManager() {
    shutdown_all();
}

size_t InstanceManager::spawn(size_t n, size_t global_deadline_ms) {
    auto t0 = std::chrono::steady_clock::now();
    size_t started = 0;
    size_t target = std::min(n, target_);
    for (size_t i = 0; i < target; ++i) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - t0).count();
        if ((size_t)elapsed > global_deadline_ms) {
            Logger::get().event("manager", "spawn.deadline", "n/a", "fail",
                                "{\"requested\":" + std::to_string(target) +
                                ",\"started\":" + std::to_string(started) + "}");
            break;
        }
        std::string id = std::to_string(i);
        auto ffi = std::make_shared<LeanFFI>();
        if (ffi->init(id, workspace_root_, repl_path_)) {
            std::lock_guard<std::mutex> g(mu_);
            instances_[id] = ffi;
            live_.fetch_add(1);
            ++started;
        } else {
            Logger::get().event(id, "spawn", "n/a", "fail",
                                "{\"reason\":\"init handshake failed\"}");
        }
    }
    Logger::get().event("manager", "spawn.batch", "evidence/manager.json", "pass",
                        "{\"requested\":" + std::to_string(target) +
                        ",\"started\":" + std::to_string(started) + "}");
    return started;
}

void InstanceManager::shutdown_all() {
    std::lock_guard<std::mutex> g(mu_);
    for (auto& [id, ffi] : instances_) {
        ffi->shutdown();
        Logger::get().event(id, "shutdown", "n/a", "pass", "{}");
    }
    instances_.clear();
    live_.store(0);
}

size_t InstanceManager::live_count() const { return live_.load(); }

std::vector<std::string> InstanceManager::live_ids() const {
    std::lock_guard<std::mutex> g(mu_);
    std::vector<std::string> ids;
    ids.reserve(instances_.size());
    for (auto& [id, _] : instances_) ids.push_back(id);
    return ids;
}

std::shared_ptr<LeanFFI> InstanceManager::acquire(const std::string& id) {
    std::lock_guard<std::mutex> g(mu_);
    auto it = instances_.find(id);
    if (it == instances_.end()) return nullptr;
    return it->second;
}

void InstanceManager::release(const std::string&) { /* no-op; ownership is shared */ }

void InstanceManager::reap() {
    // Periodic reaper stub; reserved for future use (broken pipes, etc.).
}

}  // namespace lpi
