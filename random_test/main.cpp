// lpi_random_test — spec §4.1 (Random Lean File Test Sampling) and
// spec §4.2 (addTheorem/addLemma Random Test Generation).
//
// Usage:
//   lpi_random_test --file-sample    --repl PATH --seed N --samples N
//   lpi_random_test --ffi-generated  --repl PATH --seed N --samples N
//   lpi_random_test --all            --repl PATH --seed N --samples N
//   lpi_random_test --both           (alias for --all)
//
// Each invocation:
//   1. Spawns ONE LeanFFI instance bound to its own runtime dir.
//   2. Walks /root/mycode/lean4, picks a random .lean file using a
//      splitmix64 PRNG seeded by the caller.
//   3. For --file-sample: hands the file to LeanFFI::run_file and
//      records the kernel verdict under evidence/test_sampling/.
//   4. For --ffi-generated: extracts imports + theorem statements
//      from the picked file, synthesises a `theorem lpi_thm_<hash>
//      : TYPE := by trivial` injection (or lemma), and feeds it via
//      LeanFFI::run_source. Records under evidence/ffi_generated/.
//   5. Repeats --samples times.
//
// Determinism: identical (seed, samples, mode, repl-path) reproduces
// identical evidence files byte-for-byte (modulo timestamps inside
// the JSON body, which use a deterministic source so they are also
// stable in a fixed-second sense). We log every event to
// evolution_logs/random_test.jsonl.
#include "leanffi.h"
#include "instance_manager.h"
#include "random_test.h"
#include "logger.h"
#include "evidence.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <filesystem>

using namespace lpi;

int main(int argc, char** argv) {
    Logger::get().open("random_test", "evolution_logs");
    std::string repl = "repl";
    uint64_t seed = 1;
    size_t samples = 1;
    bool do_file = false, do_ffi = false, do_all = false;
    std::string root = "/root/mycode/lean4";

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--repl" && i + 1 < argc) repl = argv[++i];
        else if (a == "--seed" && i + 1 < argc) seed = std::stoull(argv[++i]);
        else if (a == "--samples" && i + 1 < argc) samples = std::stoul(argv[++i]);
        else if (a == "--root" && i + 1 < argc) root = argv[++i];
        else if (a == "--file-sample") do_file = true;
        else if (a == "--ffi-generated") do_ffi = true;
        else if (a == "--all" || a == "--both") { do_file = true; do_ffi = true; do_all = true; }
    }
    if (!do_file && !do_ffi) {
        std::cerr << "specify --file-sample and/or --ffi-generated (or --all)\n";
        return 2;
    }

    LeanFileSampler sampler(root);
    size_t indexed = sampler.index_files();
    std::cout << "lean_root=" << root << " indexed=" << indexed << "\n";
    if (indexed == 0) {
        std::cerr << "no .lean files found under " << root << "\n";
        return 1;
    }

    // One persistent FFI per logical group; each sample is run in turn.
    LeanFFI ffi;
    std::string inst = "rt0";
    if (!ffi.init(inst, "/root/mycode/lean_physical_isolate/runtime", repl)) {
        std::cerr << "failed to init ffi against " << repl << "\n";
        return 1;
    }
    FfiGenSynthesizer synth(sampler);

    size_t file_pass = 0, file_fail = 0;
    size_t ffi_pass  = 0, ffi_fail  = 0;

    for (size_t i = 0; i < samples; ++i) {
        uint64_t s = seed + (uint64_t)i * 0x9e3779b97f4a7c15ULL;
        if (do_file) {
            LeanFileSample sample;
            if (sampler.sample_and_run(s, ffi, sample)) {
                if (sample.kernel_ok) ++file_pass; else ++file_fail;
                std::cout << "[file-sample] seed=" << s
                          << " file=" << sample.file_path
                          << " kernel_ok=" << (sample.kernel_ok ? "yes" : "no")
                          << " ev=" << sample.evidence_path << "\n";
            }
        }
        if (do_ffi) {
            FfiGenSample gs;
            if (synth.generate_and_record(s, FfiGenKind::THEOREM, gs)) {
                synth.run_against(ffi, gs);
                if (gs.kernel_ok) ++ffi_pass; else ++ffi_fail;
                std::cout << "[ffi-thm]    seed=" << s
                          << " name=" << gs.generated_name
                          << " kernel_ok=" << (gs.kernel_ok ? "yes" : "no")
                          << " ev=" << gs.evidence_path << "\n";
            }
            FfiGenSample gl;
            if (synth.generate_and_record(s + 1, FfiGenKind::LEMMA, gl)) {
                synth.run_against(ffi, gl);
                if (gl.kernel_ok) ++ffi_pass; else ++ffi_fail;
                std::cout << "[ffi-lem]    seed=" << s
                          << " name=" << gl.generated_name
                          << " kernel_ok=" << (gl.kernel_ok ? "yes" : "no")
                          << " ev=" << gl.evidence_path << "\n";
            }
        }
    }

    ffi.shutdown();
    EvidenceSystem::get().build_index();

    std::cout << "\n=== summary ===\n";
    if (do_file)
        std::cout << "file-sample: pass=" << file_pass << " fail=" << file_fail << "\n";
    if (do_ffi)
        std::cout << "ffi-generated: pass=" << ffi_pass << " fail=" << ffi_fail << "\n";
    return 0;
}
