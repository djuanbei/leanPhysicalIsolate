// Minimal Pantograph-protocol stand-in used for integration testing while
// the real Pantograph `repl` is being built. Implements only enough of the
// protocol to satisfy the LeanFFI handshake and run_source path.
//
// This file is *not* part of the production system. The real `repl` binary
// (built from /root/mycode/Pantograph) replaces it for kernel-accurate
// execution as soon as `lake build repl` finishes.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <chrono>

// A tiny table of "known" terms we can elaborate. The real repl uses
// the actual Lean kernel; we use a hash so the protocol round-trip is
// identical.
static bool fake_elaborate(const std::string& term) {
    if (term.empty()) return true;
    // We accept anything that doesn't contain an obvious syntax error.
    // This is good enough to drive the orchestrator; kernel accuracy
    // is verified separately by Validation::kernel_semantic_match()
    // against the real repl.
    for (char c : term) {
        if (c == '\n' || c == '\r') return false;
    }
    return true;
}

int main() {
    // Set HOME/TMPDIR per the LPI instance workspace if present in env.
    // Handshake
    std::cout << "ready." << std::endl;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        if (line == "quit") break;

        // Parse { "cmd": "...", "payload": ... }
        size_t cp = line.find("\"cmd\"");
        if (cp == std::string::npos) {
            std::cout << "{\"error\":\"command\",\"desc\":\"no cmd\"}" << std::endl;
            continue;
        }
        size_t cq1 = line.find('"', cp + 6);
        size_t cq2 = (cq1 == std::string::npos) ? std::string::npos : line.find('"', cq1 + 1);
        std::string cmd = (cq1 == std::string::npos || cq2 == std::string::npos) ? "" : line.substr(cq1 + 1, cq2 - cq1 - 1);

        if (cmd == "version") {
            std::cout << "{\"version\":\"0.0-mock\"}" << std::endl;
        } else if (cmd == "options set") {
            std::cout << "{}" << std::endl;
        } else if (cmd == "expr synthesize" || cmd == "run_source" || cmd.rfind("goal", 0) == 0) {
            // Extract "expr" from the payload (very loose).
            std::string expr;
            size_t ep = line.find("\"expr\"");
            if (ep != std::string::npos) {
                size_t q1 = line.find('"', ep + 6);
                size_t q2 = (q1 == std::string::npos) ? std::string::npos : line.find('"', q1 + 1);
                if (q1 != std::string::npos && q2 != std::string::npos)
                    expr = line.substr(q1 + 1, q2 - q1 - 1);
            }
            bool ok = fake_elaborate(expr);
            if (ok) {
                std::cout << "{\"type\":\"Prop\",\"expr\":\"" << expr << "\"}" << std::endl;
            } else {
                std::cout << "{\"error\":\"elab\",\"desc\":\"mock: rejected\"}" << std::endl;
            }
        } else if (cmd == "tactic") {
            std::cout << "{\"stateId\":\"s2\",\"goals\":[]}" << std::endl;
        } else if (cmd == "save") {
            std::cout << "{\"blob\":\"mock-blob\"}" << std::endl;
        } else if (cmd == "load") {
            std::cout << "{\"stateId\":\"s3\"}" << std::endl;
        } else if (cmd == "library add") {
            std::cout << "{}" << std::endl;
        } else {
            std::cout << "{\"error\":\"unknown\",\"desc\":\"" << cmd << "\"}" << std::endl;
        }
    }
    return 0;
}
