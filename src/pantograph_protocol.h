#pragma once
#include <string>

namespace lpi {

// Pantograph protocol layer.
//
// We only implement the subset of the Pantograph REPL protocol that maps
// to the spec's Execution API (§10) and Goal System (§12):
//
//   - "options set"     : configure REPL options
//   - "goal start"      : open a new goal in an environment
//   - "goal restore"    : restore a serialized goal state
//   - "tactic"          : run a single tactic on a goal state
//   - "expr synthesize" : synthesize a term
//   - "lib add"         : inject a declaration (§14)
//   - "save"            : snapshot (§15)
//   - "load"            : restore
//   - "version"         : REPL version handshake
//
// The protocol is line-delimited JSON; one request -> one response.
namespace protocol {

// Send one request to the repl (stdin pipe) and read one response line
// (stdout). The connection is held by LeanFFI; this layer is stateless.
struct Reply {
    bool ok = false;
    std::string raw;        // raw JSON line
    std::string error;      // populated when ok==false
};

// Build a request string. cmd is e.g. "goal start".
std::string build(const std::string& cmd, const std::string& payload_json_inline);

// Names of the commands we use.
namespace cmd {
    constexpr const char* kVersion     = "version";
    constexpr const char* kOptionsSet  = "options set";
    constexpr const char* kGoalStart   = "goal start";
    constexpr const char* kTactic      = "tactic";
    constexpr const char* kSave        = "save";
    constexpr const char* kLoad        = "load";
    constexpr const char* kLibAdd      = "library add";
    constexpr const char* kExprSynth   = "expr synthesize";
}  // namespace cmd

}  // namespace protocol
}  // namespace lpi
