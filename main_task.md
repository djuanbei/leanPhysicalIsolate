# Evolution Task Spec (Full Final System)

## Pantograph → Multi-Instance LeanFFI Evolution Engine

Reference system: Pantograph

Root source (immutable):

```text id="root"
/root/mycode/Pantograph
```

---

# 0. System Identity

This system defines:

> A **non-invasive, evidence-driven, Git-audited, semantically equivalent Lean execution engine** built around massive parallelism via LeanFFI.

It transforms Pantograph into:

* a 10,000-instance execution system
* a Lean-equivalent computation layer
* a stateless orchestration architecture
* an evidence-driven evolution framework
* a reproducible CMake-managed system

---

# 1. Source System (Immutable Kernel)

```text id="src"
/root/mycode/Pantograph
```

## Rules

Pantograph is a frozen upstream dependency.

### Forbidden

* modifying files
* patching
* rewriting
* injecting code
* writing build artifacts into source tree

### Allowed

* read
* analyze
* parse
* index
* build against

### Invariant

```text id="inv_src"
Pantograph(t) == Pantograph(0)
```

---

# 2. External Evolution Layer

All system evolution MUST occur outside the source tree:

```text id="ext"
/Pantograph.ext/
runtime/
scheduler/
cmake/
generated/
cache/
temp/
builds/
reports/
evolution_logs/
requirements/
```

---

# 3. Requirement System (Git-Managed Logical Layer)

Requirements define:

* scale constraints
* runtime constraints
* memory constraints
* isolation constraints
* scheduler constraints
* validation constraints
* logging constraints
* audit constraints
* source integrity constraints

---

## Requirement Structure

```text id="req"
requirements/
    core/
    ffi/
    scheduler/
    system/
    performance/
```

---

## Requirement Change Policy

Every change MUST follow:

```text id="flow_req"
Edit → Validate → Log → git add → git commit → git push
```

---

## Requirement Enforcement

Enforced by:

* CMake Project Manager
* Instance Manager
* Scheduler
* LeanFFI runtime
* validation framework

---

# 4. Git Governance

## Allowed

```bash id="git_ok"
git add
git commit
git push
git status
```

---

## Forbidden

```bash id="git_no"
git log
git show
git blame
git reflog
git shortlog
```

---

## Role

Git is:

* audit ledger
* state synchronizer
* versioned truth system

Not:

* history inspection tool

---

# 5. Logging & Audit System

```text id="logs"
/evolution_logs/
```

Each change MUST generate:

* timestamped log
* component type
* evidence reference
* validation result
* affected files
* reasoning trace

---

## Traceability Chain

```text id="trace"
Evidence → Gap → Design → Implementation → Validation → Log → Git Commit
```

---

## No Orphan Rule

Forbidden:

* unlogged changes
* unvalidated changes
* uncommitted changes
* evidence-less changes

---

# 6. Global Constraints

---

## Scale

```text id="scale"
10,000 LeanFFI instances
× 10 Lean files
= 100,000 evaluations
```

---

## Time Constraint

```text id="time"
< 5 hours total
```

Minimum throughput:

```text id="throughput"
≥ 6 eval/sec
```

---

## Memory Constraint

```text id="mem"
M(t) ≤ M(0) + ε
```

No accumulation allowed.

---

## Isolation Constraint

Each LeanFFI instance:

* independent OS process
* independent Lean runtime
* no shared state

---

# 7. LeanFFI System

---

## 7.1 Semantic Requirement (CRITICAL)

LeanFFI MUST be semantically equivalent to:

```bash id="lean"
lean
```

Formally:

```text id="eq"
∀P: LeanFFI(P) == lean(P)
```

Including:

* parsing
* elaboration
* kernel checks
* errors
* warnings
* module resolution

---

## 7.2 Execution Interfaces

### File execution

```cpp id="file"
Result run_file(path)
```

---

### Source execution

```cpp id="src"
Result run_source(code)
```

---

## 7.3 Evolution Interfaces

LeanFFI supports:

* load source string
* load file
* create goal
* remove goal
* add theorem
* add lemma
* add definition
* add structure
* add class
* add instance
* snapshot / restore / fork

These must remain Lean-semantic compatible.

---

## 7.4 Isolation Rule

No cross-instance leakage:

```text id="iso"
LeanFFI_A ≠ LeanFFI_B
```

---

# 8. Instance Manager

Responsibilities:

* spawn 10,000 LeanFFI processes
* manage lifecycle
* minimal metadata only
* no history storage

---

# 9. Scheduler

Policies:

* ROUND_ROBIN
* LEAST_LOAD
* AFFINITY
* DAG_AWARE

Rules:

* stateless or bounded state only
* streaming execution only
* no accumulation

---

# 10. CMake Project Manager

Minimum:

```text id="cmake"
CMake ≥ 3.25
```

---

## Targets

### Runtime

* spawn_10000_instances
* run_time_bounded_simulation
* memory_check
* monitor_deadline

### Validation

* validate_requirements
* validate_runtime
* validate_isolation
* validate_memory
* validate_all

### Reporting

* report_runtime
* report_memory
* report_scaling
* report_all

---

# 11. Evidence-Driven System

No change without evidence.

---

## Forbidden

* mock data
* synthetic grammar
* invented AST nodes
* fake metrics

---

# 12. Execution Pipeline

---

Phase 0: Requirements load
Phase 1: Pantograph analysis (read-only)
Phase 2: Evidence collection
Phase 3: Gap analysis
Phase 4: Design
Phase 5: Implementation (external only)
Phase 6: Validation
Phase 7: Runtime testing
Phase 8: Scaling verification
Phase 9: Memory verification
Phase 10: Logging
Phase 11: Git commit
Phase 12: Final validation

---

# 13. Observability

Allowed:

* logs
* reports
* external telemetry

Forbidden:

* persistent in-memory dashboards
* retained runtime state

---

# 14. Success Criteria

---

## Functional

* 10,000 LeanFFI instances
* 100,000 evaluations

---

## Runtime

```text id="runtime"
< 5 hours
```

---

## Memory

```text id="memok"
M(t) ≈ M(0)
```

---

## Semantic Correctness

LeanFFI must match:

```text id="sem"
lean CLI semantics exactly
```

---

## Isolation

* no cross-instance contamination

---

## Audit

* every change has:

  * evidence
  * log
  * git commit
  * validation

---

# 15. Failure Conditions

System fails if:

* runtime > 5 hours
* memory grows
* Pantograph modified
* Lean semantics diverge from `lean`
* logs missing
* evidence missing
* forbidden git commands used
* <100,000 evaluations completed

---

# 16. Optional Extensions

* distributed cluster execution
* Kubernetes orchestration
* formal verification of requirements
* cross-node LeanFFI scheduling
* proof search acceleration layer

---

# Final Definition

This system defines:

> A **fully immutable-source, semantically equivalent Lean execution engine with 10,000 isolated LeanFFI processes, Git-audited evolution, evidence-driven development, strict memory bounds, and <5-hour execution constraint**, built on top of Pantograph without modifying it.
