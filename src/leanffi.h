#pragma once
#include "result.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <mutex>

namespace lpi {

// A GoalState is a snapshot of a Pantograph metavariable context,
// including its target, local context, and universe constraints.
//
// Mirrors spec §12 (GoalState struct) and §13 (tactic evaluation).
struct GoalState {
    std::string state_id;             // stable id assigned by repl ("s{n}")
    std::vector<std::string> goals;   // sub-goal ids (multi-subgoal tactics)
    std::string target_expr;          // pretty-printed target
    std::string raw_state;            // raw JSON returned by the repl

    // Number of open subgoals in the current state.
    size_t open_goals() const { return goals.size(); }
};

// LeanFFI: thin wrapper around one Pantograph repl subprocess.
//
// We run **one repl per instance** (subprocess model). This is the only
// practical way to honour:
//   - spec §3   : filesystem isolation per instance
//   - spec §1   : immutable Pantograph (we never touch the source tree)
//   - spec §9   : kernel semantics (the repl drives the *real* kernel)
//
// Each instance has:
//   - one persistent repl subprocess (stdin/stdout JSON-RPC)
//   - a private env/ directory (cached declarations, universe constraints)
//   - a private snapshots/ directory
//   - a private goals/ directory
class LeanFFI {
public:
    LeanFFI();
    ~LeanFFI();

    // Bind this FFI to a particular instance workspace directory.
    // The repl will be launched with HOME, TMPDIR and PWD redirected
    // into the instance directory to enforce filesystem isolation.
    bool init(const std::string& instance_id,
              const std::string& workspace_root,
              const std::string& pantograph_repl_path);

    // Tear down the subprocess and flush logs.
    void shutdown();

    // ----- Execution API (spec §10) -----
    Result run_file(const std::string& path);
    Result run_source(const std::string& source);

    // ----- Goal system (spec §12, §13) -----
    Result goal_start(const std::string& expr, const std::string& type,
                      std::vector<GoalState>& out_goals);
    Result tactic(const std::string& state_id, const std::string& tactic_src,
                  std::vector<GoalState>& out_goals);

    // ----- Declaration injection (spec §14) -----
    Result add_theorem(const std::string& name,
                       const std::string& type,
                       const std::string& proof);
    Result add_lemma(const std::string& name,
                     const std::string& type,
                     const std::string& proof);
    Result add_definition(const std::string& name,
                          const std::string& type,
                          const std::string& value);
    Result add_structure(const std::string& name,
                         const std::string& fields_json);
    Result add_class(const std::string& name,
                     const std::string& extends,
                     const std::string& fields_json);
    Result add_instance(const std::string& class_name,
                        const std::string& term);

    // Lower-level: issue a single `env.add` with the explicit
    // (name, type, value, isTheorem) fields that the protocol expects.
    // Used by spec §4.2 addTheorem/addLemma synthesis testing.
    Result env_add_raw(const std::string& name,
                       const std::string& type,
                       const std::string& value,
                       bool is_theorem);

    // ----- Snapshot / Restore / Fork (spec §15) -----
    // save returns the serialized state (opaque blob).
    Result save_state(const std::string& state_id, std::string& out_blob);
    Result load_state(const std::string& blob, std::string& out_state_id);

    bool is_ready() const { return pid_ > 0; }
    const std::string& instance_id() const { return instance_id_; }
    const std::string& workspace() const { return workspace_; }

private:
    // Low-level: send one line, read one line.
    Result exchange(const std::string& req_line, std::string& out_resp_line);

    // Helpers for declaration injection.
    Result lib_add(const std::string& name,
                   const std::string& type,
                   const std::optional<std::string>& value,
                   const std::string& kind);  // "theorem" | "lemma" | "def" | "structure" | ...

    std::string instance_id_;
    std::string workspace_;
    std::string pantograph_repl_;
    int write_fd_ = -1;
    int read_fd_  = -1;
    pid_t pid_    = -1;
    bool started_ = false;
    // Serialise exchanges against this repl subprocess. The repl
    // speaks a line-oriented protocol: a concurrent write from two
    // threads will interleave bytes and the parser will see garbage
    // (spec §9 demands kernel semantics — we must not corrupt the
    // repl's input stream).
    std::mutex exchange_mu_;
};

}  // namespace lpi
