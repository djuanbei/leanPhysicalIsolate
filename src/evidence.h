#pragma once
#include <string>

namespace lpi {

// EvidenceSystem (spec §6).
//
// We don't fabricate. Each evidence file is written by the code that
// produced it. This module exists to provide a single catalogue +
// integrity check.
class EvidenceSystem {
public:
    static EvidenceSystem& get();

    // Build /evidence/INDEX.json listing every file with its size and
    // mtime. Used by the validation framework to confirm that no
    // synthetic data slipped in.
    void build_index();

    // Verify that the evidence directory is non-empty and that no
    // file is suspiciously small (< 10 bytes).
    bool verify();

    // Append a row to /evolution_logs/evidence.jsonl that records
    // every evidence file produced by an event.
    void record(const std::string& path, const std::string& event);

private:
    EvidenceSystem() = default;
};

}  // namespace lpi
