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
    for (int i = 1; i + 1 < argc; ++i) {
        std::string a = argv[i];
        if (a == "--repl") repl_path = argv[++i];
    }

    auto instances = spawn_instances(2, repl_path, {});
    if (instances.size() < 2) {
        std::cerr << "[isolation] failed to spawn 2 instances\n";
        return 1;
    }

    // Unique identifier per run so the test is repeatable
    std::string uid = "iso_test_" + std::to_string(::getpid()) + "_"
                      + std::to_string(time(nullptr));

    // Add definition to instance A
    Task add_def;
    add_def.kind = SourceKind::Source;
    add_def.content = "def " + uid + " : Nat := 12345";
    Result rA_add = instances[0]->execute(add_def);

    // Probe instance B for the same identifier — must not be defined.
    Task probe;
    probe.kind = SourceKind::Source;
    probe.content = "#check (" + uid + ")";
    Result rB_probe = instances[1]->execute(probe);

    // Probe instance A — must be defined.
    Task probeA;
    probeA.kind = SourceKind::Source;
    probeA.content = "#check (" + uid + ")";
    Result rA_probe = instances[0]->execute(probeA);

    std::ofstream out("/Pantograph.ext/reports/isolation_check.json");
    // Pass criteria:
    //   B should report an "error" (undefined identifier)
    //   A should NOT report an "error"
    bool b_isolated = (rB_probe.stdout_text.find("\"error\"") != std::string::npos);
    bool a_has_def  = (rA_probe.stdout_text.find("\"error\"") == std::string::npos);

    bool ok = b_isolated && a_has_def;
    out << "{\n"
        << "  \"uid\": \"" << uid << "\",\n"
        << "  \"a_add_response\": \"" << rA_add.stdout_text << "\",\n"
        << "  \"a_probe_response\": \"" << rA_probe.stdout_text << "\",\n"
        << "  \"b_probe_response\": \"" << rB_probe.stdout_text << "\",\n"
        << "  \"b_isolated\": " << (b_isolated ? "true" : "false") << ",\n"
        << "  \"a_has_def\": " << (a_has_def ? "true" : "false") << ",\n"
        << "  \"isolation_pass\": " << (ok ? "true" : "false") << "\n"
        << "}\n";

    std::cout << "[isolation] b_isolated=" << (b_isolated ? "T" : "F")
              << " a_has_def=" << (a_has_def ? "T" : "F")
              << " pass=" << (ok ? "T" : "F") << "\n";
    return ok ? 0 : 2;
}