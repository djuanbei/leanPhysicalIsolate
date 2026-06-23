// validate_all: runs every validation target in sequence.

#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

int main() {
    // Always use CLI backend for validations; REPL is not built.
    std::string cli_args = " --backend cli --lean /root/.elan/bin/lean";
    std::vector<std::pair<std::string, std::string>> targets = {
        {"semantic", "/Pantograph.ext/builds/validate_semantic" + cli_args},
        {"isolation", "/Pantograph.ext/builds/validate_isolation" + cli_args},
        {"requirements", "/Pantograph.ext/builds/validate_requirements"},
    };
    int failed = 0;
    for (auto& [name, cmd] : targets) {
        std::cout << "[validate_all] running " << name << " (" << cmd << ")\n";
        int rc = std::system(cmd.c_str());
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