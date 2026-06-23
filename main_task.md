# Evolution Task Spec (Full Final System)

---

# 0. System Identity

This system defines a:

> **non-invasive, evidence-driven, Git-audited, multi-instance Lean execution engine**

It transforms Pantograph into a:

* 10,000-instance LeanFFI execution platform
* fully isolated OS-level compute system
* stateless C++ orchestrator
* strict time + memory bounded system
* forward-only Git evolution environment
* log-driven audit machine

---

# 1. Source System (IMMUTABLE KERNEL)

```text
/root/mycode/Pantograph
```

### Hard Constraints:

* READ ONLY forever
* no modification
* no patching
* no injection
* no build-time writes

Pantograph is a **frozen reference kernel**

---

# 2. External Evolution Layer (ALL CHANGES LIVE HERE)

```text
/Pantograph.ext/
/runtime/
/scheduler/
/cmake/
/evolution_logs/
```

Contains:

* LeanFFI system
* scheduler
* instance manager
* CMake orchestration
* generated artifacts
* logs + evidence system

---

# 3. Requirement System (Git-Managed Truth Layer)

## 3.1 Requirement Repository

```text
/root/mycode/Pantograph.requirements/
```

---

## 3.2 Requirement Role

Defines system constraints:

* time (< 5 hours)
* memory (constant)
* scale (10,000 instances)
* isolation rules
* scheduler policies

---

## 3.3 Git Requirement Rules

Allowed:

* git add
* git commit
* git push
* git status

Forbidden:

* git log
* git show
* git blame
* git reflog
* git shortlog

---

# 4. Logging & Audit System (NEW CORE LAYER)

```text
/evolution_logs/
```

Every action MUST generate:

* timestamped log file
* evidence reference
* validation result
* Git commit association

---

## Log Rule

Every system mutation must follow:

```text
Evidence → Change → Log → Git Commit → Validation
```

---

# 5. Core Architecture

```text
[ /root/mycode/Pantograph ] (READ ONLY)
               ↓
      Evidence + Analysis Layer
               ↓
   Requirements (Git-controlled truth)
               ↓
[CMake Project Manager]
               ↓
[External Evolution Layer]
               ↓
[Instance Manager]
               ↓
[Scheduler / Router]
               ↓
[LeanFFI × 10,000 processes]
               ↓
[Lean runtime execution]
               ↓
[Streaming results only]
```

---

# 6. Global Constraints

---

## 6.1 Scale Constraint

```text
10,000 LeanFFI instances
× 10 .lean files
= 100,000 evaluations
```

---

## 6.2 Time Constraint (CRITICAL)

```text
T_total < 5 hours
```

Minimum throughput:

> ≥ 6 evaluations/sec sustained

---

## 6.3 Memory Constraint (CRITICAL)

C++ main process must remain constant:

```text
M(t) ≤ M(0) + ε
```

Rules:

* no accumulation
* no caching
* no history storage
* streaming only

---

## 6.4 Source Integrity Constraint (CRITICAL)

```text
/root/mycode/Pantograph is immutable
```

---

## 6.5 Git Constraint

Allowed only:

* add / commit / push / status

Forbidden:

* all history inspection commands

---

# 7. LeanFFI System

Each instance:

* OS process isolated
* executes 10 `.lean` files
* independent memory space
* streams results immediately
* no shared state

---

# 8. Instance Manager

Responsibilities:

* spawn 10,000 processes
* track minimal metadata only
* no long-term state storage
* crash isolation & restart

---

# 9. Scheduler / Router (Stateless Core)

Policies:

* ROUND_ROBIN
* LEAST_LOAD
* AFFINITY
* DAG_AWARE

Rules:

* no unbounded state
* streaming dispatch only
* no history retention

---

# 10. CMake Project Manager

Acts as:

* build system
* runtime orchestrator
* execution controller
* requirement loader

Targets:

* spawn_10000_instances
* run_time_bounded_simulation
* memory_check
* validate_requirements
* monitor_deadline

---

# 11. Requirement Engine (Git-driven)

Loads:

```text
/Pantograph.requirements/
```

Responsibilities:

* parse requirements
* enforce constraints
* convert into scheduler rules
* validate compliance

---

# 12. Execution Pipeline

---

## Phase 0 — Requirement Load

* load Git-tracked requirements

---

## Phase 1 — Analysis (READ ONLY)

* scan Pantograph source
* extract structure + FFI graph

---

## Phase 2 — Externalization

* build LeanFFI system outside source tree

---

## Phase 3 — Scaling

* spawn 10,000 instances

---

## Phase 4 — Execution

* 100,000 evaluations
* streaming results only

---

## Phase 5 — Validation

* time check
* memory check
* requirement compliance

---

## Phase 6 — Logging + Git Commit

* generate logs
* commit results

---

# 13. Observability Layer

Allowed:

* file logs
* external telemetry
* streaming metrics

Forbidden:

* in-memory dashboards
* accumulation buffers

---

# 14. Success Criteria

---

## 14.1 Functional

* 10,000 LeanFFI instances run
* 100,000 evaluations complete

---

## 14.2 Time

```text
< 5 hours
```

---

## 14.3 Memory

```text
M(t) ≈ M(0)
```

---

## 14.4 Requirement Compliance

* all constraints enforced from Git requirements
* no runtime-generated requirements

---

## 14.5 Source Integrity

* Pantograph unchanged forever

---

## 14.6 Audit Integrity

* every change has:

  * evidence
  * log file
  * Git commit

---

# 15. Failure Conditions

System FAILS if:

* execution exceeds 5 hours
* memory grows with instances
* source is modified
* missing logs or evidence
* forbidden Git commands used
* <100,000 evaluations completed

---

# 16. Optional Extensions

* distributed cluster execution
* Kubernetes LeanFFI fleet
* formal verification of requirements
* DAG-based proof planner
* cross-instance optimization graph
* GPU-assisted proof search

---

# FINAL SUMMARY

This system defines:

> A **fully immutable-source, Git-governed, evidence-driven, log-audited, stateless-scheduler, 10,000-instance Lean execution engine**

with:

* strict isolation (LeanFFI)
* strict time bound (< 5 hours)
* strict memory bound (constant C++ memory)
* strict forward-only Git model
* strict read-only upstream kernel (/Pantograph)


