// validate_all: runs every validation target in sequence.

#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

int main() {
    std::vector<std::pair<std::string, std::string>> targets = {
        {"semantic", "/Pantograph.ext/builds/validate_semantic"},
        {"isolation", "/Pantograph.ext/builds/validate_isolation"},
        {"requirements", "/Pantograph.ext/builds/validate_requirements"},
    };
    int failed = 0;
    for (auto& [name, path] : targets) {
        std::cout << "[validate_all] running " << name << " (" << path << ")\n";
        int rc = std::system(path.c_str());
        if (rc != 0) {
            std::cout << "[validate_all] " << name << " FAILED rc=" << rc << "\n";
            failed++;
        } else {
            std::cout << "[validate_all] " << name << " PASSED\n";
        }
    }
    std::cout << "[validate_all] " << (targets.size() - failed) << "/"
              << targets.size() << " passed\n";
    return failed == 0 ? 0 : 2;
}