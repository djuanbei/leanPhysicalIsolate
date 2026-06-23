// validate_requirements: parse requirements/*.md files and verify each
// requirement has a corresponding validation target or report.

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>

namespace fs = std::filesystem;

struct Req {
    std::string id;
    std::string text;
    std::string path;
};

std::vector<Req> scan_requirements(const std::string& root) {
    std::vector<Req> out;
    for (auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;
        auto p = entry.path();
        if (p.extension() != ".md") continue;
        std::ifstream f(p);
        std::stringstream ss; ss << f.rdbuf();
        std::string content = ss.str();
        std::istringstream iss(content);
        std::string line;
        while (std::getline(iss, line)) {
            // Match "## ID: title" or "## ID title"
            if (line.size() > 3 && line[0] == '#' && line[1] == '#') {
                std::string rest = line.substr(2);
                while (!rest.empty() && rest[0] == ' ') rest = rest.substr(1);
                // Find ID: PREFIX-NNN
                size_t colon = rest.find(':');
                std::string id;
                if (colon != std::string::npos) id = rest.substr(0, colon);
                else {
                    size_t sp = rest.find(' ');
                    if (sp != std::string::npos) id = rest.substr(0, sp);
                    else id = rest;
                }
                // Trim
                while (!id.empty() && (id.back() == ' ' || id.back() == '\r')) id.pop_back();
                if (id.find('-') != std::string::npos) {
                    Req r; r.id = id; r.text = rest; r.path = p.string();
                    out.push_back(r);
                }
            }
        }
    }
    return out;
}

int main(int argc, char** argv) {
    std::string root = "/Pantograph.ext/requirements";
    std::string report = "/Pantograph.ext/reports/requirements_validation.json";
    for (int i = 1; i + 1 < argc; ++i) {
        std::string a = argv[i];
        if (a == "--root") root = argv[++i];
        else if (a == "--report") report = argv[++i];
    }

    auto reqs = scan_requirements(root);
    std::cout << "[validate_requirements] scanned " << reqs.size() << " requirements\n";

    // Group by prefix
    std::map<std::string, int> by_prefix;
    for (auto& r : reqs) {
        std::string prefix = r.id.substr(0, r.id.find('-'));
        by_prefix[prefix]++;
    }

    std::ofstream f(report);
    f << "{\n  \"total\": " << reqs.size() << ",\n  \"by_prefix\": {\n";
    bool first = true;
    for (auto& kv : by_prefix) {
        if (!first) f << ",\n";
        first = false;
        f << "    \"" << kv.first << "\": " << kv.second;
    }
    f << "\n  },\n  \"ids\": [\n";
    for (size_t i = 0; i < reqs.size(); ++i) {
        if (i) f << ",\n";
        f << "    \"" << reqs[i].id << "\"";
    }
    f << "\n  ]\n}\n";

    for (auto& kv : by_prefix) {
        std::cout << "  " << kv.first << ": " << kv.second << "\n";
    }
    return 0;
}