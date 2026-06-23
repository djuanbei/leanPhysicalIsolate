#include "evidence.h"
#include "logger.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <iomanip>

namespace fs = std::filesystem;

namespace lpi {

EvidenceSystem& EvidenceSystem::get() {
    static EvidenceSystem e;
    return e;
}

void EvidenceSystem::build_index() {
    fs::create_directories("evidence");
    std::ofstream idx("evidence/INDEX.json");
    idx << "{\n  \"files\": [\n";
    bool first = true;
    for (auto& e : fs::recursive_directory_iterator("evidence")) {
        if (!e.is_regular_file()) continue;
        if (e.path().filename() == "INDEX.json") continue;
        if (!first) idx << ",\n";
        first = false;
        idx << "    {\"path\":\"" << e.path().string() << "\","
            << "\"size\":" << fs::file_size(e.path()) << "}";
    }
    idx << "\n  ]\n}\n";
}

bool EvidenceSystem::verify() {
    if (!fs::is_directory("evidence")) return false;
    size_t count = 0;
    for (auto& e : fs::recursive_directory_iterator("evidence")) {
        if (e.is_regular_file() && e.path().filename() != "INDEX.json") {
            if (fs::file_size(e.path()) < 10) return false;
            ++count;
        }
    }
    return count > 0;
}

void EvidenceSystem::record(const std::string& path, const std::string& event) {
    Logger::get().event("evidence", "record", path, event, "{}");
}

}  // namespace lpi
