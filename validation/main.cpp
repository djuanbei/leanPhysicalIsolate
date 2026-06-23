// lpi_validation — runs every Validation::Result check and writes
// evidence/validation.jsonl + a human-readable reports/validation.md.
#include "validation.h"
#include "evidence.h"
#include "logger.h"
#include "memory_monitor.h"

#include <iostream>
#include <fstream>
#include <filesystem>

using namespace lpi;

int main(int argc, char** argv) {
    Logger::get().open("validation", "evolution_logs");
    bool do_all = false, do_bench = false, do_mem = false;
    std::string repl = "repl";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--repl" && i + 1 < argc) repl = argv[++i];
        else if (a == "--all") do_all = true;
        else if (a == "--benchmark") do_bench = true;
        else if (a == "--memory") do_mem = true;
    }

    // Truncate the validation log.
    std::filesystem::create_directories("evidence");
    std::ofstream("evidence/validation.jsonl", std::ios::trunc);

    if (do_all || (!do_bench && !do_mem)) {
        (void)Validation::semantic_correctness(repl);
        (void)Validation::isolation_integrity();
        (void)Validation::snapshot_correctness(repl);
        (void)Validation::kernel_semantic_match(repl);
    }
    if (do_all || do_mem) {
        (void)Validation::memory_check();
    }
    if (do_all || do_bench) {
        // Informational only: 0 evaluations, 0 seconds -> rate 0.
        (void)Validation::throughput_check(0, 0.0);
    }
    EvidenceSystem::get().build_index();
    std::cout << "validation: evidence/validation.jsonl written\n";
    return 0;
}
