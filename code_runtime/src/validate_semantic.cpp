// Validation tool: runs LeanFFI semantic equivalence checks against
// the `lean` CLI for a set of source strings. Reports pass/fail.

#include "leanffi.hpp"
#include "instance_manager.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <cstdio>
#include <unistd.h>
#include <cstdlib>

namespace {

// Strip Lean REPL response to compare with `lean` output.
// For the equivalence check, we only care about the success/failure
// bit (whether elaboration produced errors).
bool response_has_error(const std::string& resp) {
    return resp.find("\"error\"") != std::string::npos;
}

// Run `lean --root=. <file>` and capture stderr+stdout. Returns whether
// the run succeeded (exit code 0).
bool run_lean_cli(const std::string& source, std::string& out_combined) {
    // Write source to a temp file in /Pantograph.ext/temp/
    std::string tmp = "/Pantograph.ext/temp/_check_" +
                      std::to_string(::getpid()) + "_" +
                      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                      ".lean";
    {
        std::ofstream f(tmp);
        f << source;
    }
    std::string cmd = "lean --root=. \"" + tmp + "\" 2>&1";
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) { ::unlink(tmp.c_str()); return false; }
    char buf[4096];
    while (fgets(buf, sizeof(buf), p)) out_combined += buf;
    int rc = pclose(p);
    ::unlink(tmp.c_str());
    return rc == 0;
}

}  // namespace

int main(int argc, char** argv) {
    using namespace leanffi;

    std::string repl_path = "/root/mycode/Pantograph/.lake/build/bin/repl";
    std::string lean_path = "/root/.elan/bin/lean";
    std::string backend = "cli";   // default: CLI mode (always available)
    std::string report_dir = "/Pantograph.ext/reports";
    std::vector<std::string> cases;

    // Default equivalence test cases (small).
    cases.push_back("#check (1 : Nat)");
    cases.push_back("#check (Nat.add 2 3)");
    cases.push_back("theorem t1 : 1 + 1 = 2 := by norm_num");
    cases.push_back("def foo : Nat := 42");

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--repl") repl_path = argv[++i];
        else if (a == "--lean") lean_path = argv[++i];
        else if (a == "--backend") backend = argv[++i];
    }

    std::cout << "[validate] backend=" << backend << " cases=" << cases.size() << "\n";

    // Spawn 1 instance for LeanFFI side
    auto instances = (backend == "cli")
        ? spawn_instances_cli(1, lean_path, std::vector<std::string>{})
        : spawn_instances(1, repl_path, std::vector<std::string>{});
    if (instances.empty()) {
        std::cerr << "[validate] failed to spawn LeanFFI instance\n";
        return 1;
    }
    auto& ffi = *instances[0];

    int passed = 0, failed = 0;
    std::ofstream report(report_dir + "/semantic_equivalence.jsonl");
    for (size_t i = 0; i < cases.size(); ++i) {
        // Compare with `lean` CLI
        std::string lean_out;
        bool lean_ok = run_lean_cli(cases[i], lean_out);

        // LeanFFI side
        Task t;
        t.task_id = (int64_t)i;
        t.kind = SourceKind::Source;
        t.content = cases[i];
        Result r = ffi.execute(t);
        bool ffi_ok = r.success;

        bool match = (lean_ok == ffi_ok);
        if (match) passed++; else failed++;

        std::cout << "[validate] case " << i
                  << " lean_ok=" << (lean_ok ? "T" : "F")
                  << " ffi_ok=" << (ffi_ok ? "T" : "F")
                  << " match=" << (match ? "T" : "F") << "\n";

        report << "{"
               << "\"case\":" << i
               << ",\"lean_ok\":" << (lean_ok ? "true" : "false")
               << ",\"ffi_ok\":" << (ffi_ok ? "true" : "false")
               << ",\"match\":" << (match ? "true" : "false")
               << ",\"ffi_wall_ms\":" << (int64_t)(r.wall_seconds * 1000)
               << "}\n";
    }

    std::ofstream sum(report_dir + "/semantic_equivalence_summary.json");
    sum << "{\n"
        << "  \"cases\": " << cases.size() << ",\n"
        << "  \"passed\": " << passed << ",\n"
        << "  \"failed\": " << failed << ",\n"
        << "  \"pass_rate\": " << (cases.empty() ? 0.0 : (double)passed / cases.size()) << "\n"
        << "}\n";

    std::cout << "[validate] " << passed << "/" << cases.size() << " passed\n";
    return failed == 0 ? 0 : 2;
}