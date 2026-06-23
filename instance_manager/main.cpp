// lpi_instance_manager — spec target spawn_10000_instances
//
// Usage:
//   lpi_instance_manager --spawn-all [--repl PATH] [--target N] [--active-cap N]
//   lpi_instance_manager --spawn N
//   lpi_instance_manager --shutdown
//   lpi_instance_manager --list
#include "instance_manager.h"
#include "logger.h"
#include "evidence.h"

#include <iostream>
#include <cstdlib>
#include <string>

using namespace lpi;

int main(int argc, char** argv) {
    Logger::get().open("manager", "evolution_logs");
    InstanceManager mgr;

    std::string repl = "repl";        // resolved via PATH
    size_t target = 10000;
    size_t active = 64;               // bounded-memory cap (host-tuned)
    bool do_spawn_all = false;
    bool do_shutdown = false;
    bool do_list = false;
    long spawn_n = -1;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--repl" && i + 1 < argc) repl = argv[++i];
        else if (a == "--target" && i + 1 < argc) target = std::stoul(argv[++i]);
        else if (a == "--active-cap" && i + 1 < argc) active = std::stoul(argv[++i]);
        else if (a == "--spawn-all") do_spawn_all = true;
        else if (a == "--spawn" && i + 1 < argc) spawn_n = std::stol(argv[++i]);
        else if (a == "--shutdown") do_shutdown = true;
        else if (a == "--list") do_list = true;
    }

    mgr.set_repl_path(repl);
    mgr.set_target(target);
    mgr.set_active_cap(active);

    if (do_shutdown) { mgr.shutdown_all(); return 0; }
    if (do_list) {
        for (auto& id : mgr.live_ids()) std::cout << id << "\n";
        return 0;
    }
    size_t started = 0;
    if (do_spawn_all) started = mgr.spawn_all();
    else if (spawn_n >= 0) started = mgr.spawn((size_t)spawn_n);
    else { std::cerr << "no action\n"; return 2; }

    std::cout << "started=" << started << " target=" << target
              << " active_cap=" << active << "\n";
    EvidenceSystem::get().build_index();
    return 0;
}
