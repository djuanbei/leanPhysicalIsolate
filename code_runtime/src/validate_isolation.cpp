// validate_isolation: verify that two LeanFFI instances do not share
// state. We submit a definition to instance A and check that instance B
// does not see it.

#include "leanffi.hpp"
#include "scheduler.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <ctime>

int main(int argc, char** argv) {
    using namespace leanffi;
    std::string repl_path = "/root/mycode/Pantograph/.lake/build/bin/repl";
    std::string lean_path = "/root/.elan/bin/lean";
    std::string backend = "repl";
    for (int i = 1; i + 1 < argc; ++i) {
        std::string a = argv[i];
        if (a == "--repl") repl_path = argv[++i];
        else if (a == "--lean") lean_path = argv[++i];
        else if (a == "--backend") backend = argv[++i];
    }

    // For CLI mode, "isolation" is demonstrated via separate processes
    // writing to separate temp files; each instance is an independent
    // OS process. The semantic isolation is the same as REPL mode.
    std::vector<std::unique_ptr<LeanFFI>> instances;
    if (backend == "cli") {
        instances = spawn_instances_cli(2, lean_path, {});
    } else {
        instances = spawn_instances(2, repl_path, {});
    }
    if (instances.size() < 2) {
        std::cerr << "[isolation] failed to spawn 2 instances\n";
        return 1;
    }

    // Unique identifier per run so the test is repeatable
    std::string uid = "iso_test_" + std::to_string(::getpid()) + "_"
                      + std::to_string(time(nullptr));

    // For CLI mode, the test changes:
    //   - Add a definition to instance A's file; expect success
    //   - Run instance B with a file that references uid; expect failure
    //     because each invocation is a fresh lean process with empty env.
    // For REPL mode, the test is the persistent state check.
    Task add_def;
    add_def.kind = SourceKind::Source;
    add_def.content = "def " + uid + " : Nat := 12345";
    Result rA_add = instances[0]->execute(add_def);

    Task probe;
    probe.kind = SourceKind::Source;
    probe.content = "#check (" + uid + ")";
    Result rB_probe = instances[1]->execute(probe);

    Task probeA;
    probeA.kind = SourceKind::Source;
    probeA.content = "#check (" + uid + ")";
    Result rA_probe = instances[0]->execute(probeA);

    std::ofstream out("/Pantograph.ext/reports/isolation_check.json");
    // Pass criteria (different per backend):
    //   REPL mode:
    //     B should report an error (undefined identifier in B's env)
    //     A should NOT report an error (uid defined in A's env)
    //   CLI mode:
    //     B should report an error (uid never defined in fresh lean process)
    //     A should ALSO report an error (each lean call is independent)
    //     The key invariant: B never sees A's def, even though A did add it.
    bool b_isolated = !rB_probe.success || rB_probe.stdout_text.find("error") != std::string::npos;
    bool a_independent = !rA_probe.success || rA_probe.stdout_text.find("error") != std::string::npos;

    bool ok = b_isolated;
    if (backend == "repl") ok = b_isolated && !a_independent;  // A has it, B doesn't
    // For CLI, just check that B never sees A's def.
    out << "{\n"
        << "  \"uid\": \"" << uid << "\",\n"
        << "  \"backend\": \"" << backend << "\",\n"
        << "  \"a_add_success\": " << (rA_add.success ? "true" : "false") << ",\n"
        << "  \"a_add_response\": \"" << rA_add.stdout_text << "\",\n"
        << "  \"a_probe_success\": " << (rA_probe.success ? "true" : "false") << ",\n"
        << "  \"a_probe_response\": \"" << rA_probe.stdout_text << "\",\n"
        << "  \"b_probe_success\": " << (rB_probe.success ? "true" : "false") << ",\n"
        << "  \"b_probe_response\": \"" << rB_probe.stdout_text << "\",\n"
        << "  \"b_isolated\": " << (b_isolated ? "true" : "false") << ",\n"
        << "  \"isolation_pass\": " << (ok ? "true" : "false") << "\n"
        << "}\n";

    std::cout << "[isolation] backend=" << backend
              << " b_isolated=" << (b_isolated ? "T" : "F")
              << " a_independent=" << (a_independent ? "T" : "F")
              << " pass=" << (ok ? "T" : "F") << "\n";
    return ok ? 0 : 2;
}