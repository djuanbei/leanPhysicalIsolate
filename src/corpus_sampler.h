#pragma once
#include <string>
#include <vector>
#include <random>
#include <optional>

namespace leanffi {

struct SampledFile {
    std::string full_path;
    std::string rel_path;
    std::string hash;
    uint64_t size_bytes = 0;
    std::string source;
};

// Picks random .lean files from a corpus directory tree.
// Uses deterministic seed for reproducibility.
class CorpusSampler {
public:
    explicit CorpusSampler(std::string root, uint64_t seed = 0xc0ffeeULL);

    void scan(); // walks root and collects .lean files

    // Returns a random file. If avoid list is non-empty, prefers files not in it.
    std::optional<SampledFile> sample(const std::vector<std::string>& avoid_hashes = {});

    size_t total() const { return files_.size(); }
    const std::string& root() const { return root_; }

private:
    std::string root_;
    uint64_t seed_;
    std::mt19937_64 rng_;
    std::vector<SampledFile> files_;
    void walk(const std::string& dir);
};

}