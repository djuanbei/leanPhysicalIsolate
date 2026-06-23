# Evolution Task Spec (v4 — Final Consolidated System)

## Pantograph → Physical-Isolated Multi-Instance LeanFFI Evolution Engine

---

# 0. System Identity

This system defines:

> A physically isolated, evidence-driven, Git-audited Lean kernel–semantically equivalent execution engine built on an immutable Pantograph base, executing large-scale parallel Lean workloads through 10,000 isolated LeanFFI instances.

It provides:

* Lean kernel–equivalent execution (NOT CLI semantics)
* 10,000 isolated OS-level Lean runtimes
* interactive theorem manipulation engine
* snapshot / restore / fork system
* bounded-memory execution model
* reproducible CMake orchestration
* full filesystem-based isolation

---

# 1. Immutable Source Kernel

Root:

```text
/root/mycode/Pantograph
```

## Invariant

```text
Pantograph(t) = Pantograph(0)
```

## Forbidden

* modify source
* patch source
* inject code
* write artifacts into source tree

## Allowed

* read
* analyze
* compile against
* link against
* index

---

# 2. Physical Isolation Workspace (ROOT SYSTEM)

All evolution MUST occur inside:

```text
/root/mycode/lean_physical_isolate
```

---

## Workspace Structure

```text
/root/mycode/lean_physical_isolate/

runtime/
instance_manager/
scheduler/
leanffi/
validation/
evidence/
evolution_logs/
requirements/
snapshots/
forks/
benchmarks/
reports/
generated/
cache/
cmake/
```

---

## Hard Constraint

```text
ALL runtime state, logs, snapshots, and artifacts MUST remain inside lean_physical_isolate
```

---

# 3. Isolation Model (CORE PROPERTY)

Each instance:

```text
/root/mycode/lean_physical_isolate/runtime/instance_<id>/
```

Structure:

```text
instance_<id>/
  env/
  goals/
  logs/
  cache/
  snapshots/
```

---

## Isolation Invariant

```text
∀ i ≠ j:
State(i) ∩ State(j) = ∅
```

Only immutable shared artifacts are allowed.

---

# 4. Requirement System

Path:

```text
/root/mycode/lean_physical_isolate/requirements/
```

Structure:

* core/
* runtime/
* ffi/
* scheduler/
* memory/
* validation/
* audit/

---

## Requirement Lifecycle

```text
Edit → Validate → Evidence → Log → git commit
```

---

# 5. Git Governance

Git is an audit ledger only.

Allowed:

```bash
git add
git commit
git status
```

Forbidden:

```bash
git log
git diff (for reasoning/history)
git blame
```

---

# 6. Evidence System

Path:

```text
/root/mycode/lean_physical_isolate/evidence/
```

Rules:

* only real runtime outputs allowed
* no synthetic data
* no hallucinated proofs
* no fabricated benchmarks

---

# 7. Logging System

Path:

```text
/root/mycode/lean_physical_isolate/evolution_logs/
```

Each event MUST include:

* timestamp
* instance id
* operation type
* evidence reference
* validation result

---

## Traceability Chain

```text
Evidence → Gap → Design → Implementation → Validation → Log → Git Commit
```

---

# 8. Global Constraints

## Scale

```text
10,000 LeanFFI instances
```

---

## Runtime

```text
< 3 hours total
```

---

## Throughput

```text
≥ 6 evaluations/sec
```

---

## Memory

```text
M_active(t) ≤ M0 + ε
```

Snapshots excluded from active memory accounting.

---

# 9. LeanFFI Semantic Model (CRITICAL)

LeanFFI must match:

```text
Lean kernel semantics (elaborator + kernel + tactic engine)
```

NOT CLI behavior.

---

## Core Semantic Function

```text
(Environment × Command)
→ (Environment × Diagnostics)
```

---

## Tactic Semantics

```text
(GoalState × Tactic)
→ (GoalState × Diagnostics)
```

---

# 10. Execution API

```cpp
Result run_file(path);
Result run_source(source);
```

Execution MUST occur inside:

```text
/root/mycode/lean_physical_isolate/runtime/
```

---

# 11. Environment Model

Stored per instance:

```text
instance_<id>/env/
```

Contains:

* declarations
* imports
* universe constraints
* elaboration state
* kernel state snapshot

---

# 12. Goal System (FORMAL)

```cpp
struct GoalState {
  metavariables;
  local_context;
  target_expression;
  universe_constraints;
  environment_ref;
}
```

---

# 13. Tactic Evaluation

```text
GoalState evaluate(goal, tactic)
```

Must support:

* multiple subgoals
* failure diagnostics
* kernel-accurate behavior

---

# 14. Declaration Injection

All must pass Lean kernel elaboration.

* add_theorem
* add_lemma
* add_definition
* add_structure
* add_class
* add_instance

---

# 15. Snapshot / Restore / Fork

## Snapshot

Stored at:

```text
/root/mycode/lean_physical_isolate/snapshots/
```

Captures:

* environment
* metavariables
* goals
* universe constraints

---

## Restore

```text
restore(snapshot) = original environment
```

---

## Fork

Creates new isolated instance:

```text
/root/mycode/lean_physical_isolate/runtime/instance_<new_id>/
```

---

# 16. Instance Manager

Responsibilities:

* spawn 10,000 processes
* enforce filesystem isolation
* lifecycle management
* cleanup

---

# 17. Scheduler

Policies:

* ROUND_ROBIN
* LEAST_LOAD
* DAG_AWARE

Constraints:

* bounded memory
* no cross-instance state
* streaming execution only

---

# 18. Validation Framework

Checks:

* semantic correctness
* isolation integrity
* memory usage
* runtime throughput
* snapshot correctness

---

# 19. CMake System

Path:

```text
/root/mycode/lean_physical_isolate/cmake/
```

Targets:

* spawn_10000_instances
* run_parallel_execution
* validate_all
* benchmark_all
* memory_check

---

# 20. Observability

Allowed:

* logs
* traces
* benchmark outputs

Forbidden:

* persistent dashboards
* global state retention

---

# 21. Execution Pipeline

1. Load requirements
2. Analyze Pantograph (read-only)
3. Collect evidence
4. Gap analysis
5. Design
6. Implement
7. Validate
8. Run at scale
9. Memory check
10. Semantic verification
11. Log results
12. Git commit
13. Final audit

---

# 22. Success Criteria

## Functional

* 10,000 LeanFFI instances
* ≥100,000 evaluations

---

## Runtime

```text
< 3 hours
```

---

## Memory

```text
M_active(t) ≈ M0
```

---

## Semantic

```text
LeanFFI ≡ Lean kernel semantics
```

---

## Isolation

* zero cross-instance leakage
* filesystem enforced separation

---

# 23. Failure Conditions

System fails if:

* Pantograph modified
* runtime exceeds limit
* memory unbounded
* semantic mismatch with Lean kernel
* missing evidence/logs
* snapshot restore inconsistency
* fork contamination
* isolation violation
* evaluation target not met

---

# FINAL DEFINITION

A physically isolated, evidence-driven, Git-audited Lean kernel–semantically equivalent execution engine built on Pantograph, operating entirely inside:

```text
/root/mycode/lean_physical_isolate
```

providing:

* 10,000 isolated LeanFFI runtimes
* interactive theorem manipulation
* snapshot / restore / fork semantics
* strict filesystem isolation
* bounded memory execution
* reproducible CMake orchestration
* large-scale parallel validation

```
```
