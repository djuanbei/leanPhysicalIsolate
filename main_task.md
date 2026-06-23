# Evolution Task Specification (Full Final System v2)

## Pantograph → Multi-Instance LeanFFI Evolution Engine

Reference system:

```text
Pantograph
```

Immutable upstream source:

```text
/root/mycode/Pantograph
```

---

# 0. System Identity

This system defines:

> A fully immutable-source, evidence-driven, Git-audited, semantically equivalent Lean execution platform built on Pantograph and evolved externally through LeanFFI, capable of massive parallel execution, interactive theorem manipulation, environment snapshots, proof search, and large-scale validation.

The system transforms Pantograph into:

* a 10,000-instance Lean execution platform
* a Lean-semantic computation layer
* an interactive theorem engineering system
* a parallel proof-search platform
* a reproducible CMake-managed infrastructure
* an evidence-driven evolution framework

---

# 1. Immutable Source Kernel

Root source:

```text
/root/mycode/Pantograph
```

Pantograph remains immutable.

## Forbidden

* modifying source files
* patching source files
* rewriting source files
* injecting source code
* altering Pantograph build scripts
* storing artifacts inside source tree

## Allowed

* read
* analyze
* parse
* index
* compile against
* link against

Invariant:

```text
Pantograph(t) == Pantograph(0)
```

---

# 2. External Evolution Layer

All evolution occurs outside Pantograph.

```text
/tmp/Pantograph.ext/

runtime/
scheduler/
instance_manager/
leanffi/
validation/
cmake/
generated/
cache/
temp/
builds/
reports/
evolution_logs/
requirements/
evidence/
benchmarks/
tests/
```

No generated artifact may enter the Pantograph tree.

---

# 3. Requirement Management System

Requirements are the authoritative logical specification.

Requirements define:

* runtime constraints
* semantic constraints
* memory constraints
* scheduler constraints
* isolation constraints
* validation constraints
* observability constraints
* audit constraints
* source integrity constraints

Structure:

```text
requirements/

core/
ffi/
scheduler/
runtime/
performance/
memory/
validation/
audit/
```

---

## Requirement Change Workflow

Every requirement modification must follow:

```text
Edit
→ Validate
→ Log
→ git add
→ git commit
→ git push
```

---

## Requirement Enforcement

Requirements are enforced by:

* CMake Project Manager
* LeanFFI Runtime
* Scheduler
* Instance Manager
* Validation Framework

---

# 4. Git Governance

Git acts as:

* audit ledger
* synchronization mechanism
* versioned truth system

Allowed:

```bash
git add
git commit
git push
git status
```

Forbidden:

```bash
git log
git show
git blame
git reflog
git shortlog
```

---

# 5. Evidence System

No modification without evidence.

Directory:

```text
evidence/
```

Evidence sources:

* Pantograph source analysis
* Lean source analysis
* runtime measurements
* memory measurements
* benchmark results
* validation failures
* proof-search outcomes

Forbidden:

* fabricated evidence
* synthetic benchmarks
* invented measurements
* invented declarations
* fake validation results

---

# 6. Logging and Audit

Directory:

```text
evolution_logs/
```

Every change generates:

* timestamp
* component identifier
* evidence references
* reasoning trace
* validation results
* modified files

Traceability chain:

```text
Evidence
→ Gap
→ Design
→ Implementation
→ Validation
→ Log
→ Git Commit
```

No orphan changes allowed.

Forbidden:

* unlogged changes
* unvalidated changes
* evidence-less changes
* uncommitted changes

---

# 7. Global Constraints

## Scale

```text
10,000 LeanFFI processes
× 10 Lean files
=
100,000 evaluations
```

---

## Runtime

```text
Total runtime < 3 hours
```

Required throughput:

```text
≥ 6 evaluations/sec
```

---

## Memory

Invariant:

```text
M(t) ≤ M(0) + ε
```

No unbounded accumulation.

---

## Isolation

Each LeanFFI instance:

* separate OS process
* separate Lean runtime
* separate environment
* separate goals
* separate snapshots

No shared mutable state.

---

# 8. LeanFFI Semantic Contract

Critical invariant:

```text
∀P

LeanFFI(P)
=
lean(P)
```

LeanFFI must match Lean CLI semantics exactly.

Including:

* parser behavior
* elaboration
* type checking
* kernel verification
* imports
* module resolution
* diagnostics
* warnings
* errors

---

# 9. LeanFFI Execution Layer

## File Execution

```cpp
Result run_file(path);
```

Equivalent to:

```bash
lean file.lean
```

---

## Source Execution

```cpp
Result run_source(source);
```

Equivalent to:

```bash
echo source | lean
```

---

# 10. LeanFFI Interactive Environment API

LeanFFI must support persistent theorem environments.

---

## Environment Loading

```cpp
EnvironmentHandle load_source(
    source);
```

```cpp
EnvironmentHandle load_file(
    path);
```

---

## Goal Management

### Create Goal

```cpp
GoalHandle create_goal(
    EnvironmentHandle env,
    proposition);
```

Example:

```lean
⊢ Nat.add_comm a b
```

---

### Remove Goal

```cpp
bool remove_goal(
    GoalHandle goal);
```

---

# 11. Tactic Evaluation API

Interactive theorem proving support.

```cpp
GoalState evaluate(
    GoalHandle goal,
    tactic);
```

Examples:

```lean
simp
rw [Nat.add_comm]
exact h
aesop
omega
```

Returns:

```cpp
GoalState
```

containing:

* solved status
* remaining goals
* diagnostics

---

# 12. Declaration Injection API

Environment evolution operations.

---

## Add Theorem

```cpp
add_theorem(
    env,
    theorem_source);
```

---

## Add Lemma

```cpp
add_lemma(
    env,
    lemma_source);
```

---

## Add Definition

```cpp
add_definition(
    env,
    definition_source);
```

---

## Add Structure

```cpp
add_structure(
    env,
    structure_source);
```

---

## Add Class

```cpp
add_class(
    env,
    class_source);
```

---

## Add Instance

```cpp
add_instance(
    env,
    instance_source);
```

All declarations must pass normal Lean elaboration and kernel verification.

---

# 13. Snapshot and Fork System

Required for proof exploration.

---

## Snapshot

```cpp
SnapshotHandle snapshot(
    env);
```

Captures:

* environment
* imports
* declarations
* options
* goals

---

## Restore

```cpp
EnvironmentHandle restore(
    snapshot);
```

Invariant:

```text
restore(snapshot(E))
=
E
```

---

## Fork

```cpp
EnvironmentHandle fork(
    env);
```

Invariant:

```text
semantic(fork(E))
=
semantic(E)
```

Subsequent mutations are isolated.

---

# 14. Introspection API

Required for evolution and validation.

```cpp
list_declarations()

get_declaration(name)

list_goals()

get_goal(goal_id)

get_environment_stats()

get_runtime_stats()

get_diagnostics()
```

---

# 15. Serialization API

Required for persistence.

```cpp
serialize_environment()

deserialize_environment()

export_module()

import_module()
```

---

# 16. Runtime Control API

```cpp
reset_runtime()

shutdown_runtime()

restart_runtime()

health_check()
```

---

# 17. Instance Manager

Responsibilities:

* spawn 10,000 LeanFFI processes
* restart failed instances
* monitor health
* track minimal metadata
* avoid history retention

Forbidden:

* proof history accumulation
* environment accumulation
* scheduler-owned theorem state

---

# 18. Scheduler

Policies:

* ROUND_ROBIN
* LEAST_LOAD
* AFFINITY
* DAG_AWARE

Requirements:

* bounded memory
* streaming execution
* stateless or bounded state

Forbidden:

* global theorem caches
* unbounded queues
* retained execution history

---

# 19. Validation Framework

Validation targets:

```text
validate_requirements
validate_runtime
validate_memory
validate_semantics
validate_isolation
validate_scheduler
validate_ffi
validate_all
```

---

# 20. Runtime Benchmarks

Benchmark categories:

* startup latency
* evaluation throughput
* memory growth
* snapshot cost
* restore cost
* fork cost
* tactic execution cost
* declaration insertion cost

All benchmark results must be evidence-backed.

---

# 21. CMake Project Manager

Minimum version:

```text
CMake ≥ 3.25
```

---

## Runtime Targets

```text
spawn_10000_instances
run_massive_parallel_execution
run_time_bounded_simulation
memory_check
monitor_deadline
```

---

## Validation Targets

```text
validate_requirements
validate_runtime
validate_semantics
validate_memory
validate_isolation
validate_all
```

---

## Reporting Targets

```text
report_runtime
report_memory
report_scaling
report_semantics
report_all
```

---

# 22. Observability

Allowed:

* logs
* reports
* telemetry
* benchmark artifacts

Forbidden:

* persistent runtime state
* retained theorem history
* long-lived dashboards

---

# 23. Execution Pipeline

Phase 0:
Requirements Load

Phase 1:
Pantograph Analysis

Phase 2:
Evidence Collection

Phase 3:
Gap Analysis

Phase 4:
Architecture Design

Phase 5:
Implementation

Phase 6:
Validation

Phase 7:
Runtime Testing

Phase 8:
Scaling Verification

Phase 9:
Memory Verification

Phase 10:
Semantic Verification

Phase 11:
Logging

Phase 12:
Git Commit

Phase 13:
Final Validation

---

# 24. Success Criteria

Functional:

* 10,000 LeanFFI processes
* 100,000 evaluations

Runtime:

```text
< 5 hours
```

Memory:

```text
M(t) ≈ M(0)
```

Semantics:

```text
LeanFFI == lean CLI
```

Interactive APIs:

* create_goal
* remove_goal
* evaluate
* add theorem
* add lemma
* add definition
* add structure
* add class
* add instance
* snapshot
* restore
* fork

must all satisfy Lean semantic equivalence.

Audit:

Every change has:

* evidence
* validation
* log
* git commit

---

# 25. Failure Conditions

System fails if:

* runtime > 5 hours
* memory grows unbounded
* Pantograph modified
* Lean semantics diverge
* logs missing
* evidence missing
* validation missing
* forbidden git commands used
* fewer than 100,000 evaluations complete
* goal API diverges from Lean behavior
* snapshot/restore loses semantics
* fork leaks state across instances

---

# 26. Optional Future Extensions

* distributed cluster execution
* Kubernetes orchestration
* proof-search acceleration
* theorem synthesis engine
* reinforcement-guided tactic search
* formally verified scheduler
* formally verified LeanFFI semantics
* cross-node environment migration

---

# Final Definition

The system is a fully immutable-source, evidence-driven, Git-audited, semantically Lean-equivalent execution platform built on Pantograph, providing 10,000 isolated LeanFFI runtimes, interactive theorem-environment manipulation, snapshot/restore/fork capabilities, bounded-memory execution, reproducible CMake infrastructure, and large-scale parallel validation without modifying Pantograph.
