# Evolution Task Spec (Full Final System)

## Pantograph → Multi-Instance LeanFFI Evolution Engine

Reference system: Pantograph

Reference source tree:

```text
/root/mycode/Pantograph
```

---

# 0. System Identity

This system defines a:

> **Non-invasive, evidence-driven, Git-audited, multi-instance LeanFFI execution platform**

that transforms Pantograph into a:

* massively parallel Lean execution engine
* process-isolated Lean runtime platform
* stateless orchestration system
* evidence-driven evolution system
* reproducible CMake-managed infrastructure

---

# 1. Source System (Immutable Kernel)

Reference repository:

```text
/root/mycode/Pantograph
```

## Source Integrity Rules

Pantograph is a frozen upstream dependency.

### Forbidden

* modifying source files
* patching files
* rewriting files
* code injection
* generated outputs inside source tree
* build-time writes into source tree

### Allowed

* read
* scan
* parse
* analyze
* index
* build against

### Invariant

```text
Pantograph(t) == Pantograph(0)
```

for all execution times.

---

# 2. External Evolution Layer

All evolution occurs outside Pantograph.

Example layout:

```text
Pantograph.ext/
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

# 3. Requirement System

## 3.1 Requirement Authority

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

Requirements are version-controlled artifacts and participate in the normal evolution workflow.

---

## 3.2 Requirement Organization

Example structure:

```text
requirements/
    core/
    ffi/
    scheduler/
    system/
    performance/
```

---

## 3.3 Requirement Categories

### Core

* scale
* memory
* runtime
* isolation

### FFI

* LeanFFI lifecycle
* communication
* routing

### Scheduler

* dispatch policy
* load balancing
* throttling

### System

* source integrity
* Git policy
* logging policy

### Performance

* throughput
* latency
* scaling

---

## 3.4 Requirement Change Policy

Every requirement modification must follow:

```text
Edit
→ Validate
→ Log
→ Git Add
→ Git Commit
→ Git Push
```

---

## 3.5 Requirement Validation

Requirements must be:

* internally consistent
* machine-readable
* auditable
* reproducible
* validation-backed

---

## 3.6 Requirement Enforcement

Requirements must be enforced by:

* CMake Project Manager
* Instance Manager
* Scheduler
* LeanFFI Runtime
* Validation Framework

---

# 4. Git Governance

## Allowed Commands

```bash
git add
git commit
git push
git status
```

---

## Forbidden Commands

```bash
git log
git show
git blame
git reflog
git shortlog
```

---

## Forward-Only Evolution Rule

Git is used as:

* storage
* synchronization
* audit ledger

Git is NOT used for:

* history inspection
* commit analysis
* blame tracking

---

# 5. Logging & Audit Layer

Directory:

```text
evolution_logs/
```

---

## Mandatory Logging

Every evolution event must generate:

* timestamp
* component
* evidence
* validation result
* affected files
* reason
* next action

---

## Traceability Chain

Every change must satisfy:

```text
Evidence
→ Gap
→ Design
→ Implementation
→ Validation
→ Log
→ Git Commit
```

---

## No Orphan Rule

Forbidden:

* change without evidence
* change without log
* change without validation
* change without Git commit

---

# 6. Global Constraints

## Scale Constraint

```text
10,000 LeanFFI instances
× 10 Lean files
=
100,000 evaluations
```

---

## Runtime Constraint

```text
Total Runtime < 5 hours
```

Includes:

* startup
* scheduling
* execution
* cleanup
* validation

Required throughput:

```text
≥ 6 evaluations/sec
```

---

## Memory Constraint

Main orchestration process:

```text
M(t) ≤ M(0) + ε
```

Where ε is bounded.

### Forbidden

* unbounded queues
* retained evaluation history
* result accumulation
* persistent runtime caches

---

## Isolation Constraint

Every LeanFFI instance:

* independent process
* independent memory space
* independent Lean runtime
* no shared mutable state

---

# 7. LeanFFI Architecture

## Core Principle

Each LeanFFI object represents:

> One physical isolated Lean execution environment

---

## LeanFFI Responsibilities

* load Lean runtime
* execute Lean requests
* return streamed results
* cleanup resources

---

## LeanFFI Properties

```text
LeanFFI Instance
    ↕
One OS Process
    ↕
One Lean Runtime
```

---

# 8. Instance Manager

Responsibilities:

* create LeanFFI instances
* destroy instances
* restart failed instances
* track minimal metadata

---

## Scaling Target

```text
10000 instances
```

---

## State Restriction

Forbidden:

* execution history storage
* large metadata caches
* retained outputs

---

# 9. Scheduler

## Supported Policies

* ROUND_ROBIN
* LEAST_LOAD
* AFFINITY
* DAG_AWARE

---

## Scheduler Rules

Must:

* dispatch tasks
* maintain bounded state
* stream outputs

Must not:

* accumulate results
* keep historical execution records

---

# 10. CMake Project Manager

Minimum Version:

```text
CMake >= 3.25
```

---

## Build Model

Only:

```text
Out-of-source build
```

Example:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

---

## Required Targets

### Runtime

```text
spawn_10000_instances
run_time_bounded_simulation
memory_check
monitor_deadline
```

### Validation

```text
validate_requirements
validate_runtime
validate_isolation
validate_memory
validate_all
```

### Reporting

```text
report_runtime
report_memory
report_scaling
report_all
```

---

# 11. Evidence-Driven Evolution

## Fundamental Rule

```text
No evidence → No change
```

---

## Evidence Sources

Real repository data only.

Examples:

* Lean source files
* runtime execution
* validation outputs
* coverage reports

---

## Forbidden

* mock evidence
* fabricated metrics
* invented grammar
* invented AST nodes
* synthetic validation

---

# 12. Execution Pipeline

## Phase 0

Requirement Loading

## Phase 1

Pantograph Analysis (read-only)

## Phase 2

Evidence Collection

## Phase 3

Gap Analysis

## Phase 4

Design

## Phase 5

Implementation (external layer only)

## Phase 6

Validation

## Phase 7

Runtime Testing

## Phase 8

Scaling Verification

## Phase 9

Memory Verification

## Phase 10

Logging

## Phase 11

Git Commit

## Phase 12

Final Validation

---

# 13. Observability

### Allowed

* file logs
* reports
* telemetry streams
* external monitoring

### Forbidden

* unbounded in-memory dashboards
* retained runtime state

---

# 14. Success Criteria

## Functional

* 10,000 LeanFFI instances created
* 100,000 evaluations completed

---

## Runtime

```text
< 5 hours
```

---

## Memory

```text
M(t) ≈ M(0)
```

---

## Isolation

* no cross-instance contamination
* no shared runtime state

---

## Source Integrity

Pantograph unchanged.

---

## Git Compliance

Only approved commands used.

---

## Audit Compliance

Every change is:

* evidence-backed
* validated
* logged
* committed

---

# 15. Failure Conditions

System fails if:

* runtime exceeds 5 hours
* memory grows with workload
* source tree modified
* evidence missing
* logs missing
* validation missing
* forbidden Git commands used
* fewer than 100,000 evaluations completed

---

# 16. Optional Future Extensions

* distributed cluster execution
* Kubernetes orchestration
* remote worker pools
* proof-search acceleration
* formal verification integration
* cross-machine LeanFFI scheduling

---

# Final Definition

This system defines:

> A **read-only Pantograph-based, Git-governed, evidence-driven, CMake-managed, multi-instance LeanFFI execution platform**

with:

* 10,000 isolated LeanFFI processes
* 100,000 Lean evaluations
* execution time under 5 hours
* constant-memory orchestration
* immutable Pantograph source tree
* mandatory evidence, validation, logging, and Git-audited evolution workflow.
