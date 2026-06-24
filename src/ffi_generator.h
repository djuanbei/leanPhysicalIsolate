#pragma once
#include <string>
#include <vector>
#include "corpus_sampler.h"

namespace leanffi {

struct FfiTest {
    std::string kind;          // "addTheorem" | "addLemma" | "addDefinition"
    std::string name;
    std::string type;
    std::string value;         // typically "by sorry" or extracted proof
    std::string source_file;   // rel path of originating file
    std::string source_hash;
    std::string import_hint;   // optional module name
    std::vector<std::string> levels;
};

// Reads real Lean source and synthesizes kernel-typable addTheorem/addLemma calls
// that target the Pantograph env.add command. Strict invariants:
//  - names use only valid Lean identifier chars
//  - types are simple Prop or concrete expressions
//  - values default to "by sorry" or stripped theorem-statement-as-value
class FfiGenerator {
public:
    FfiGenerator() = default;

    // generate one or more addTheorem/addLemma tests from a sampled file
    std::vector<FfiTest> generate(const SampledFile& file, size_t max_per_file = 3) const;

private:
    bool is_valid_ident(const std::string& s) const;
    std::string clean_type(const std::string& s) const;
    std::string clean_ident(const std::string& s) const;
};

}