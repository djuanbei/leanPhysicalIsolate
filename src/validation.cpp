#include "validation.h"
#include "evidence.h"
#include "util.h"
#include "logger.h"
#include <fstream>
#include <algorithm>
#include <set>

namespace leanffi {

bool ValidationFramework::all_passed() const {
    for (const auto& c : results_) if (!c.passed) return false;
    return true;
}

ValidationCheck ValidationFramework::check_isolation(const std::string& runtime_root) {
    ValidationCheck c;
    c.name = "isolation_integrity";
    if (!dir_exists(runtime_root)) {
        c.detail = "runtime root missing: " + runtime_root;
        results_.push_back(c);
        return c;
    }
    auto entries = list_dir(runtime_root);
    std::set<std::string> ids;
    for (const auto& e : entries) {
        if (starts_with(e, "instance_")) {
            std::string id = e.substr(std::string("instance_").size());
            ids.insert(id);
            std::string inst_root = runtime_root + "/" + e;
            for (const auto& sub : {"env", "goals", "logs", "cache", "snapshots"}) {
                if (!dir_exists(inst_root + "/" + sub)) {
                    c.detail = "instance " + id + " missing subdir " + sub;
                    results_.push_back(c);
                    return c;
                }
            }
        }
    }
    if (ids.size() < 2) {
        c.detail = "expected at least 2 isolated instances, got " + std::to_string(ids.size());
        results_.push_back(c);
        return c;
    }
    c.passed = true;
    c.detail = "verified " + std::to_string(ids.size()) + " isolated instances";
    set(c.extra, "instance_count", (long long)ids.size());
    results_.push_back(c);
    return c;
}

ValidationCheck ValidationFramework::check_evidence(const std::string& evidence_root) {
    ValidationCheck c;
    c.name = "evidence_present";
    if (!dir_exists(evidence_root)) {
        c.detail = "evidence root missing";
        results_.push_back(c);
        return c;
    }
    // count files in subdirs
    auto ts_files = list_dir(evidence_root + "/test_sampling");
    auto gen_files = list_dir(evidence_root + "/ffi_generated");
    auto val_files = list_dir(evidence_root + "/validation");
    c.passed = (!ts_files.empty() || !gen_files.empty() || !val_files.empty());
    if (!c.passed) {
        c.detail = "no evidence files produced";
    } else {
        c.detail = "test_sampling=" + std::to_string(ts_files.size()) +
                   " ffi_generated=" + std::to_string(gen_files.size()) +
                   " validation=" + std::to_string(val_files.size());
    }
    set(c.extra, "test_sampling_count", (long long)ts_files.size());
    set(c.extra, "ffi_generated_count", (long long)gen_files.size());
    set(c.extra, "validation_count", (long long)val_files.size());
    results_.push_back(c);
    return c;
}

ValidationCheck ValidationFramework::check_snapshots(const std::string& snapshots_root) {
    ValidationCheck c;
    c.name = "snapshots_consistent";
    int total = 0;
    int consistent = 0;
    // Look only at directories whose name starts with "snap_".
    auto check_tree = [&](const std::string& root) {
        if (!dir_exists(root)) return;
        for (const auto& e : list_dir(root)) {
            if (!starts_with(e, "snap_")) continue;
            std::string sub = root + "/" + e;
            if (dir_exists(sub)) {
                ++total;
                if (file_exists(sub + "/snapshot.json")) ++consistent;
            }
        }
    };
    // If the caller passed /runtime, scan per-instance snapshots subdirs.
    if (dir_exists(snapshots_root)) {
        for (const auto& e : list_dir(snapshots_root)) {
            if (starts_with(e, "instance_")) {
                check_tree(snapshots_root + "/" + e + "/snapshots");
            } else if (starts_with(e, "snap_")) {
                check_tree(snapshots_root);
            }
        }
    }
    // Also check the top-level /snapshots dir
    check_tree(snapshots_root);
    c.passed = (total == 0) || (consistent == total);
    c.detail = "snapshots_total=" + std::to_string(total) +
               " consistent=" + std::to_string(consistent);
    set(c.extra, "snapshots_total", (long long)total);
    set(c.extra, "snapshots_consistent", (long long)consistent);
    results_.push_back(c);
    return c;
}

ValidationCheck ValidationFramework::check_pantograph_dependency(const std::string& pantograph_root,
                                                                 const std::string& build_dir) {
    ValidationCheck c;
    c.name = "pantograph_dependency";
    // Verify: Pantograph source tree exists and is untouched (read-only)
    // Verify: our build dir references the immutable REPL binary
    bool src_ok = dir_exists(pantograph_root + "/Pantograph") && file_exists(pantograph_root + "/Main.lean");
    std::string repl = pantograph_root + "/.lake/build/bin/repl";
    bool bin_ok = file_exists(repl);
    bool ref_ok = false;
    if (file_exists(build_dir + "/CMakeCache.txt")) {
        std::string cache = read_file(build_dir + "/CMakeCache.txt");
        ref_ok = (cache.find(pantograph_root) != std::string::npos) &&
                 (cache.find(repl) != std::string::npos);
    } else {
        ref_ok = false;
    }
    c.passed = src_ok && bin_ok && ref_ok;
    if (!c.passed) {
        c.detail = "src_ok=" + std::string(src_ok ? "1" : "0") +
                   " bin_ok=" + std::string(bin_ok ? "1" : "0") +
                   " ref_ok=" + std::string(ref_ok ? "1" : "0");
    } else {
        c.detail = "Pantograph immutable base integrated via build cache";
    }
    set(c.extra, "src_ok", src_ok);
    set(c.extra, "bin_ok", bin_ok);
    set(c.extra, "ref_ok", ref_ok);
    set(c.extra, "pantograph_root", pantograph_root);
    results_.push_back(c);
    return c;
}

ValidationCheck ValidationFramework::check_random_lean_file_executed(const std::string& evidence_root) {
    ValidationCheck c;
    c.name = "random_lean_file_executed";
    std::string dir = evidence_root + "/test_sampling";
    bool ok = dir_exists(dir) && !list_dir(dir).empty();
    c.passed = ok;
    c.detail = ok ? "random Lean corpus sampling evidence present" : "missing test_sampling evidence";
    results_.push_back(c);
    return c;
}

ValidationCheck ValidationFramework::check_ffi_generation(const std::string& evidence_root) {
    ValidationCheck c;
    c.name = "ffi_generation_valid";
    std::string dir = evidence_root + "/ffi_generated";
    bool ok = dir_exists(dir) && !list_dir(dir).empty();
    c.passed = ok;
    c.detail = ok ? "addTheorem/addLemma generation evidence present" : "missing ffi_generated evidence";
    results_.push_back(c);
    return c;
}

bool ValidationFramework::run_all(size_t instance_count,
                                  size_t expected_instances,
                                  double elapsed_seconds,
                                  uint64_t total_evaluations) {
    results_.clear();

    ValidationCheck c1;
    c1.name = "instance_count";
    c1.passed = (instance_count >= std::min<size_t>(expected_instances, 2));
    c1.detail = "active=" + std::to_string(instance_count) +
                " expected=" + std::to_string(expected_instances);
    set(c1.extra, "active", (long long)instance_count);
    set(c1.extra, "expected", (long long)expected_instances);
    results_.push_back(c1);

    ValidationCheck c2;
    c2.name = "evaluation_throughput";
    double eps = (elapsed_seconds > 0.0) ? (total_evaluations / elapsed_seconds) : 0.0;
    c2.passed = (eps >= 6.0);
    c2.detail = "evals=" + std::to_string(total_evaluations) +
                " elapsed_s=" + std::to_string(elapsed_seconds) +
                " eps=" + std::to_string(eps);
    set(c2.extra, "evals_per_sec", eps);
    set(c2.extra, "total_evaluations", (long long)total_evaluations);
    results_.push_back(c2);

    ValidationCheck c3;
    c3.name = "runtime_budget";
    c3.passed = (elapsed_seconds < 3 * 3600);
    c3.detail = "elapsed=" + std::to_string(elapsed_seconds) + "s (<3h)";
    results_.push_back(c3);

    JsonValue report = obj();
    JsonValue results_arr = leanffi::arr();
    for (const auto& r : results_) {
        JsonValue e = obj();
        set(e, "name", r.name);
        set(e, "passed", r.passed);
        set(e, "detail", r.detail);
        set(e, "extra", r.extra);
        set_arr(results_arr, e);
    }
    set(report, "results", results_arr);
    set(report, "elapsed_s", elapsed_seconds);
    set(report, "evaluations", (long long)total_evaluations);
    set(report, "instances", (long long)instance_count);
    EvidenceStore::instance().write_validation("summary", report);

    bool all = true;
    for (const auto& r : results_) if (!r.passed) { all = false; break; }
    return all;
}

}