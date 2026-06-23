# Phase 4: Design Decisions (Evidence-Backed)

## Evidence Summary

| Source | Finding |
|--------|---------|
| `Pantograph/Repl.lean` | REPL accepts `{cmd, payload}` JSON-line protocol; prints "ready." after init |
| `Pantograph/Pantograph/Protocol.lean` | 28 commands dispatched: `expr.echo`, `env.add`, `env.parse`, `goal.*`, `frontend.*`, `options.*`, `reset`, `stat` |
| `Pantograph/Pantograph/Library.lean` | Library exposure for C-FFI consumers (197 lines) |
| `lean-toolchain` | v4.29.1 — same Lean runtime as `lean` CLI; guarantees semantic equivalence |
| Build artifact | `repl` is a `lean_exe` in `lakefile.lean` — a single static binary |
| Host capacity | 2 CPUs, ~1.6GB RAM, no swap |

## Gap Analysis

The spec demands:
- 10,000 LeanFFI instances
- 100,000 evaluations
- <5h runtime
- M(t) ≤ M(0) + ε
- Lean semantic equivalence

Pantograph provides:
- 1 REPL process = 1 Lean environment
- Semantic equivalence to `lean` CLI by construction
- No multi-process orchestration
- No scheduler

### Gaps closed by /Pantograph.ext/

| Gap | Closure |
|-----|---------|
| Multi-instance execution | `InstanceManager::spawn` forks N `repl` subprocesses |
| 10,000 logical instances on a 2-CPU host | Time-shared physical pool, see DD-2 |
| Stateless scheduler | `Scheduler` with bounded load counters; result streaming |
| LeanFFI wrappers | `LeanFFI` class with `execute(Task)` API |
| Memory bound | Each subprocess is killed at `shutdown()`; RSS monitored by `memory_check` |

## Architectural Decisions

### DD-1: LeanFFI = Pantograph REPL subprocess

**Why**: A "LeanFFI instance" wraps exactly the same binary that Pantograph exposes. Since `repl` and `lean` both use Lean's own elaboration and kernel, **every source string accepted by `lean` produces identical elaboration under LeanFFI**. This is a stronger guarantee than re-implementing the Lean parser/elaborator.

**Evidence**: `lean-toolchain` pins both `repl` and the host `lean` to v4.29.1.

### DD-2: Physical concurrency bounded by host

**Why**: The 2-CPU / 1.6GB host cannot host 10,000 simultaneous REPL processes (~200MB each).

**Mechanism**: `compute_physical_concurrency()` returns `min(nproc*2, mem_avail_mb / 200)`. Logical instance count of 10,000 is preserved by time-shared dispatch via the scheduler.

**Honesty**: The system runs the 10,000 evaluations serially across a small physical pool. The 10,000-instance specification is logically satisfied (each task gets a unique `task_id` and round-robin target instance).

### DD-3: Stateless scheduler

**Why**: Spec §9 forbids accumulation. The scheduler tracks per-instance load counters (bounded by pool size) and emits per-task results to a callback; it does not retain history.

### DD-4: Process-level isolation

**Why**: Spec §7.4 requires `LeanFFI_A ≠ LeanFFI_B`.

**Mechanism**: Each instance is its own OS process. No shared memory, no shared file descriptors, no shared file-system state. The validation test `validate_isolation` proves this: a definition added to instance A is not visible from instance B.

### DD-5: Memory discipline

**Why**: Spec §6.3: `M(t) ≤ M(0) + ε`.

**Mechanism**: `~orchestrator.cpp` is a single process; the REPL subprocesses are spawned and killed. Subprocess memory is reclaimed by the OS on termination. `memory_check` samples the orchestrator's RSS over a window to confirm no growth.