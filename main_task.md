
# Evolution Task Spec

---

# 0. Source System (READ-ONLY)

```text id="src"
/root/mycode/Pantograph
```

### Absolute rule:

> This directory is **immutable forever**

* No modification
* No patching
* No rewriting
* No build-time or runtime writes

It is a **frozen reference kernel**

---

# 1. Mission

Transform Pantograph into:

> A **CMake-orchestrated, multi-instance LeanFFI execution engine**
> operating as a **non-invasive external evolution layer**

Capabilities:

* 10,000 isolated LeanFFI processes
* 100,000 `.lean` evaluations
* < 5 hours total execution
* zero memory growth in C++ orchestrator
* forward-only Git system
* read-only upstream source

---

# 2. Global System Architecture

```text id="arch"
[ /root/mycode/Pantograph ]   (READ ONLY)
              ↓
     analysis / introspection only
              ↓
[ External Evolution Layer ]
   (/Pantograph.ext/, /runtime/, /cmake/)
              ↓
[CMake Project Manager]
              ↓
[Instance Manager]
              ↓
[Scheduler / Router]
              ↓
[LeanFFI × 10,000 OS processes]
              ↓
[Lean runtime execution]
              ↓
[streamed results only]
```

---

# 3. Hard Global Constraints

---

## 3.1 Scale Constraint

```text id="scale"
10,000 LeanFFI instances
× 10 .lean files each
= 100,000 evaluations
```

---

## 3.2 Time Constraint (CRITICAL)

```text id="time"
T_total < 5 hours
```

Must include:

* spawn time
* scheduling
* execution
* cleanup

Minimum throughput:

> ≥ 6 evaluations/sec sustained

---

## 3.3 Memory Constraint (CRITICAL)

C++ main process MUST NOT grow:

```text id="mem"
∀t: M(t) ≤ M(0) + ε
```

Rules:

* no accumulation buffers
* no stored results
* no history retention
* streaming-only design

---

## 3.4 Source Integrity Constraint (CRITICAL NEW)

```text id="src_lock"
/root/mycode/Pantograph is immutable
```

### Forbidden:

* file edits
* patching
* injection
* in-place refactoring
* build system writes into source tree

---

## 3.5 Git Constraint (STRICT)

### Allowed:

* git add
* git commit
* git push
* git status

### Forbidden:

* git log
* git show
* git blame
* git reflog
* git shortlog

System is **forward-only evolution**

---

# 4. Core System Components

---

## 4.1 CMake Project Manager

Acts as:

* build system
* runtime orchestrator
* evolution controller
* simulation runner

Key targets:

* spawn_10000_instances
* run_time_bounded_simulation
* memory_check
* monitor_deadline
* adaptive_throttle

---

## 4.2 External Evolution Layer (NEW CORE)

All modifications exist here:

```text id="ext"
/Pantograph.ext/
/runtime/
/cmake/
/scheduler/
```

Contains:

* LeanFFI implementation
* scheduler
* instance manager
* routing layer
* analysis outputs

---

## 4.3 LeanFFI (Atomic Execution Unit)

Each instance:

* OS process (fully isolated)
* independent Lean runtime
* executes 10 `.lean` files sequentially
* streams results immediately
* no internal buffering

---

## 4.4 Instance Manager

Manages:

* 10,000 processes
* lifecycle (spawn/kill/restart)
* minimal metadata only
* no historical accumulation

---

## 4.5 Scheduler / Router (Stateless Core)

Policies:

* ROUND_ROBIN
* LEAST_LOAD
* AFFINITY
* DAG_AWARE

Properties:

* stateless or bounded state only
* no memory growth
* streaming dispatch only

---

## 4.6 LeanFFI Layer

Replaces direct execution coupling:

```text id="ffi"
Pantograph → LeanFFI → Scheduler → External system → Streamed response
```

---

## 4.7 Evolution Pipeline Engine

### Phase 1 — Analysis

* read-only scan of `/root/mycode/Pantograph`
* extract FFI graph

### Phase 2 — Externalization

* build external LeanFFI layer
* no source modification

### Phase 3 — Scale Enablement

* spawn 10,000 instances

### Phase 4 — Execution

* 100,000 evaluations
* streaming-only processing

### Phase 5 — Optimization

* scheduler tuning
* throughput stabilization

---

# 5. Execution Model

```text id="exec"
Pantograph (read-only)
      ↓
CMake orchestration
      ↓
InstanceManager spawns 10,000 LeanFFI processes
      ↓
Scheduler distributes tasks
      ↓
Lean executes .lean files
      ↓
Results streamed immediately
      ↓
Main process discards all data
```

---

# 6. Observability (External Only)

Allowed:

* file logs
* external telemetry
* streaming metrics

Forbidden:

* in-memory dashboards
* aggregation buffers
* runtime history storage

---

# 7. Success Criteria

---

## 7.1 Functional Success

* [ ] 10,000 LeanFFI instances spawn successfully
* [ ] 100,000 `.lean` evaluations completed
* [ ] zero interference across instances

---

## 7.2 Memory Success (CRITICAL)

```text id="mem_success"
M(t) ≈ M(0)
```

---

## 7.3 Time Success (CRITICAL)

```text id="time_success"
T_total < 5 hours
```

* ≥ 6 evaluations/sec sustained

---

## 7.4 Isolation Success

* no shared state across instances
* full process-level isolation verified

---

## 7.5 Git Success

* only allowed commands used
* no history inspection ever executed

---

## 7.6 Source Integrity Success (NEW CRITICAL)

* `/root/mycode/Pantograph` remains unchanged at all times

---

# 8. Failure Conditions

System FAILS if:

* execution > 5 hours
* memory grows with instance count
* scheduler accumulates state
* source directory modified
* fewer than 100,000 evaluations complete
* throughput < 6 eval/sec
* forbidden git commands executed

---

# 9. Optional Extensions

* distributed multi-node LeanFFI cluster
* Kubernetes orchestration layer
* GPU-assisted proof search
* checkpointing (external only)
* cross-instance optimization graph
* DAG-based proof planning engine

---

# Final System Summary

This system defines a:

> **non-invasive, forward-only, memory-bounded, time-constrained, massively parallel Lean execution engine**

with:

* 10,000 isolated LeanFFI processes
* 100,000 evaluations in < 5 hours
* zero mutation of `/root/mycode/Pantograph`
* zero memory growth in orchestrator
* strict Git discipline

---

If you want next step, I can generate:

* a **real CMake + external overlay repository layout**
* or a **10,000-process Linux scaling blueprint (ulimit + kernel tuning)**
* or a **production-grade LeanFFI dispatcher implementation (C++ + IPC)**

