#include "corpus_sampler.h"
#include "util.h"
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <algorithm>
#include <sstream>

namespace leanffi {

CorpusSampler::CorpusSampler(std::string root, uint64_t seed)
    : root_(std::move(root)), seed_(seed), rng_(seed) {}

void CorpusSampler::walk(const std::string& dir) {
    DIR* d = opendir(dir.c_str());
    if (!d) return;
    struct dirent* de;
    while ((de = readdir(d))) {
        std::string name = de->d_name;
        if (name == "." || name == "..") continue;
        std::string full = dir + "/" + name;
        struct stat st;
        if (stat(full.c_str(), &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            // skip lake/build dirs
            if (name == ".lake" || name == "build" || name == ".git" ||
                name == "node_modules" || name == "target") continue;
            walk(full);
        } else if (S_ISREG(st.st_mode)) {
            if (ends_with(name, ".lean")) {
                SampledFile f;
                f.full_path = full;
                f.rel_path = full.substr(root_.size() + 1);
                f.size_bytes = static_cast<uint64_t>(st.st_size);
                f.hash = sha256_of_file(full);
                if (st.st_size <= 256 * 1024) {
                    f.source = read_file(full);
                } else {
                    // large files: take head only, for safety
                    std::string s = read_file(full);
                    if (s.size() > 256 * 1024) s.resize(256 * 1024);
                    f.source = s;
                }
                files_.push_back(std::move(f));
            }
        }
    }
    closedir(d);
}

void CorpusSampler::scan() {
    files_.clear();
    walk(root_);
    std::sort(files_.begin(), files_.end(),
              [](const SampledFile& a, const SampledFile& b) { return a.rel_path < b.rel_path; });
}

std::optional<SampledFile> CorpusSampler::sample(const std::vector<std::string>& avoid_hashes) {
    if (files_.empty()) return std::nullopt;
    // try non-avoided first, otherwise pick any
    std::vector<size_t> prefer;
    std::vector<size_t> fallback;
    for (size_t i = 0; i < files_.size(); ++i) {
        bool avoid = false;
        for (const auto& h : avoid_hashes) if (files_[i].hash == h) { avoid = true; break; }
        if (avoid) fallback.push_back(i);
        else prefer.push_back(i);
    }
    std::vector<size_t>* pool = prefer.empty() ? &fallback : &prefer;
    if (pool->empty()) return std::nullopt;
    size_t idx = (*pool)[rng_() % pool->size()];
    return files_[idx];
}

}