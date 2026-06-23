// lpi_leanffi — minimal CLI exposing the LeanFFI API directly.
//   lpi_leanffi --repl PATH --instance ID run-source "True"
//   lpi_leanffi --repl PATH --instance ID goal-start "True" "True"
#include "leanffi.h"
#include "logger.h"
#include "instance_manager.h"

#include <iostream>

using namespace lpi;

int main(int argc, char** argv) {
    Logger::get().open("leanffi", "evolution_logs");
    std::string repl = "repl", instance = "cli", action, a, b;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        if (k == "--repl" && i + 1 < argc) repl = argv[++i];
        else if (k == "--instance" && i + 1 < argc) instance = argv[++i];
        else if (k == "run-source" && i + 1 < argc) { action = k; a = argv[++i]; }
        else if (k == "goal-start" && i + 2 < argc) { action = k; a = argv[++i]; b = argv[++i]; }
    }
    if (action.empty()) { std::cerr << "no action\n"; return 2; }

    LeanFFI ffi;
    if (!ffi.init(instance, "/root/mycode/lean_physical_isolate/runtime", repl)) {
        std::cerr << "init failed\n";
        return 1;
    }
    if (action == "run-source") {
        auto r = ffi.run_source(a);
        std::cout << (r.ok ? "OK\n" : "FAIL ") << r.raw_json << "\n";
    } else if (action == "goal-start") {
        std::vector<GoalState> gs;
        auto r = ffi.goal_start(a, b, gs);
        std::cout << (r.ok ? "OK\n" : "FAIL ") << r.raw_json << "\n";
    }
    return 0;
}
