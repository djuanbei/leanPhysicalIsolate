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
* randomized Lean corpus validation + theorem synthesis testing

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

# 2. Physical Isolation Workspace

All evolution MUST occur inside:

```text
/root/mycode/lean_physical_isolate
```

## Structure

```text
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

# 3. Isolation Model

Each instance:

```text
/root/mycode/lean_physical_isolate/runtime/instance_<id>/
```

Structure:

```text
env/
goals/
logs/
cache/
snapshots/
```

## Isolation Invariant

```text
∀ i ≠ j:
State(i) ∩ State(j) = ∅
```

---

# 4. Requirement System

Root:

```text
/root/mycode/lean_physical_isolate/requirements/
```

Lifecycle:

```text
Edit → Validate → Evidence → Log → Git Commit
```

---

## 4.1 Random Lean File Test Sampling

### Source

```text
/root/mycode/lean4
```

### Rule

Each validation cycle MUST:

* randomly select `.lean` file
* execute it via LeanFFI

```text
run_file(f) OR run_source(read(f))
```

### Constraints

* deterministic seed required
* fully reproducible
* logged in evidence system

### Evidence

```text
evidence/test_sampling/<timestamp>_<file_hash>.json
```

---

## 4.2 addTheorem / addLemma Random Test Generation

### Purpose

Generate dynamic LeanFFI test cases from real Lean code.

### Pipeline

1. Random `.lean` file selection:

```text
/root/mycode/lean4
```

2. Extract semantic context:

* imports
* definitions
* structures
* theorem statements

3. Generate test injection:

```text
addTheorem(...)
addLemma(...)
```

4. Execute via LeanFFI:

```cpp
run_source(generated_snippet)
```

### Generation Rules

* Must use symbols from file or imports
* Must remain kernel-typable
* May use `by sorry` or extracted proof patterns
* Must preserve semantic consistency

### Evidence Output

```text
evidence/ffi_generated/<timestamp>_<file_hash>.json
```

---

# 5. Git Governance

Allowed:

```bash
git add
git commit
git status
```

Forbidden:

```bash
git log
git diff
git blame
```

---

# 6. Evidence System

```text
evidence/
```

Rules:

* only real runtime outputs
* no synthetic proofs
* no fabricated benchmarks

---

# 7. Logging System

```text
evolution_logs/
```

Each event MUST include:

* timestamp
* instance id
* operation type
* evidence reference
* validation result

---

# 8. Global Constraints

* 10,000 LeanFFI instances
* < 3 hours runtime
* ≥ 6 evaluations/sec

```text
M_active(t) ≤ M0 + ε
```

---

# 9. LeanFFI Semantic Model

Must match Lean kernel semantics:

* elaborator
* kernel
* tactic engine

NOT CLI behavior.

---

## 9.1 Pantograph Dependency Invariant (NEW)

All LeanFFI functionality MUST be derived from and delegated to the immutable base system Pantograph.

### Core Requirement

```text
LeanFFI functions MUST NOT reimplement any functionality already provided by Pantograph.
```

### Rules

* LeanFFI is a thin orchestration layer over Pantograph
* All semantic core operations MUST forward to Pantograph APIs

### Forbidden

* Reimplementing kernel / elaborator / tactic engine
* Shadow semantics duplicating Pantograph behavior
* Bypassing Pantograph execution paths

### Allowed

* Scheduling
* isolation management
* snapshot / fork / restore logic
* logging and evidence generation
* orchestration of multi-instance execution

### Semantic Invariant

```text
LeanFFI(x) = Compose(Pantograph(x))
```

### Failure Condition

System is invalid if:

* any kernel logic is reimplemented in LeanFFI
* Pantograph linkage is bypassed
* execution cannot be traced back to Pantograph

---

## Core Function

```text
(Environment × Command) → (Environment × Diagnostics)
```

---

## Tactic Model

```text
(GoalState × Tactic) → (GoalState × Diagnostics)
```

---

# 10. Execution API

```cpp
Result run_file(path);
Result run_source(source);
```

Must execute inside:

```text
/root/mycode/lean_physical_isolate/runtime/
```

---

# 11. Environment Model

Per instance:

* declarations
* imports
* universe constraints
* elaboration state
* kernel snapshot

---

# 12. Goal System

```cpp
struct GoalState {
  metavariables;
  local_context;
  target_expression;
  universe_constraints;
  environment_ref;
};
```

---

# 13. Tactic Evaluation

* multiple subgoals
* failure diagnostics
* kernel-accurate semantics

---

# 14. Declaration Injection

Supported:

* addTheorem
* addLemma
* addDefinition
* addStructure
* addClass
* addInstance

---

# 15. Snapshot / Restore / Fork

## Snapshot

```text
snapshots/
```

## Restore

Restores exact environment state

## Fork

Creates new isolated instance:

```text
runtime/instance_<new_id>/
```

---

# 16. Instance Manager

* spawn 10,000 instances
* enforce isolation
* lifecycle control
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
* streaming execution

---

# 18. Validation Framework

Checks:

* semantic correctness
* isolation integrity
* memory usage
* throughput
* snapshot correctness
* random Lean file execution
* theorem/lemma generation validity
* Pantograph dependency compliance (no duplication)

---

# 19. CMake System

```text
cmake/
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
8. Random Lean file execution
9. Generate addTheorem/addLemma tests
10. Run at scale
11. Memory check
12. Semantic verification
13. Log results
14. Git commit (add project files + evolution report)
15. Final audit

---

# 22. Success Criteria

## Functional

* 10,000 LeanFFI instances
* ≥100,000 evaluations
* randomized Lean corpus coverage
* theorem/lemma synthesis validation

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
* filesystem isolation enforced

---

# 23. Failure Conditions

System fails if:

* Pantograph modified
* runtime exceeds limit
* memory unbounded
* kernel semantic mismatch
* missing evidence/logs
* snapshot inconsistency
* fork contamination
* isolation violation
* random Lean file test not executed
* theorem/lemma generation invalid or missing
* LeanFFI reimplements Pantograph functionality

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
* randomized Lean4 corpus execution
* automatic addTheorem/addLemma synthesis testing
* enforced Pantograph dependency boundary (no reimplementation of core kernel logic)

