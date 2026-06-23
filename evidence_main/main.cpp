// lpi_evidence — produces evidence/INDEX.json and verifies the evidence
// directory is non-empty and uncorrupted.
#include "evidence.h"
#include <iostream>
#include <filesystem>

using namespace lpi;

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    std::filesystem::create_directories("evidence");
    EvidenceSystem::get().build_index();
    bool ok = EvidenceSystem::get().verify();
    std::cout << "evidence_index=evidence/INDEX.json verified=" << (ok ? "yes" : "no") << "\n";
    return ok ? 0 : 1;
}
