// lpi_requirements — runs the requirements evaluator and writes
// evidence/requirements.jsonl.
#include "requirements.h"
#include "evidence.h"
#include "logger.h"

#include <iostream>

using namespace lpi;

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    Logger::get().open("requirements", "evolution_logs");
    Requirements::get().evaluate_all();
    EvidenceSystem::get().build_index();
    std::cout << "requirements: evidence/requirements.jsonl written\n";
    return 0;
}
