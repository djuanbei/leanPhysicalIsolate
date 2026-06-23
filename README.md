# lean_physical_isolate

A physically isolated, evidence-driven, Git-audited LeanFFI evolution
engine built on top of the [Pantograph](https://github.com/lenianiva/Pantograph)
repl, executing parallel Lean workloads through many isolated
`repl` subprocesses.

This workspace implements the spec in `main_task.md`. The Pantograph
source tree at `/root/mycode/Pantograph` is treated as an immutable
base — we read it, build it, and link against its `repl` binary, but
we never modify, patch, or write into it.

## Layout

```
lean_physical_isolate/
├── main_task.md            # the spec (untouched)
├── build.sh                # builds Pantograph repl + this C++ layer
├── run_pipeline.sh         # non-interactive end-to-end run
├── repl                    # the Pantograph repl (built, copied here)
├── build/                  # cmake build dir
├── src/                    # C++ implementation
├── instance_manager/       # main.cpp
├── scheduler/              # main.cpp
├── leanffi/                # main.cpp
├── validation/             # main.cpp
├── evidence/               # main.cpp
├── cmake/                  # CMakeLists.txt
├── runtime/instance_<id>/  # one per live instance
├── snapshots/              # snapshot archives + replay logs
├── forks/                  # forked instances
├── evidence/               # real runtime outputs
├── evolution_logs/         # JSON-line event stream
├── requirements/           # REQ-NNN.md files
└── reports/                # human-readable summaries
```

## Build

```bash
bash build.sh
```

This runs `lake build repl` inside the (read-only) Pantograph tree, copies
the binary to `./repl`, then builds the C++ orchestration.

## Run

```bash
bash run_pipeline.sh 8 8      # 8 instances, 8 evaluations
# or directly:
./build/lpi_instance_manager --repl ./repl --target 8 --active-cap 8 --spawn-all
./build/lpi_scheduler       --repl ./repl --target 8 --active-cap 8 \
                            --policy ROUND_ROBIN --dispatch 8 --workers 4
./build/lpi_validation      --repl ./repl --all
```

## Spec mapping

| spec section | file |
|---|---|
| §1 immutable kernel | `build.sh` (read-only `lake build`) |
| §2 workspace | this directory tree |
| §3 isolation | `src/instance_manager.cpp` (chdir+HOME+TMPDIR per subprocess) |
| §4 requirements | `requirements/<area>/REQ-NNN.md` + `src/requirements.cpp` |
| §5 git | `git` is used as an audit ledger by the user; only `add/commit/status` |
| §6 evidence | `evidence/` + `src/evidence.cpp` |
| §7 logs | `evolution_logs/global.jsonl` + per-instance streams |
| §8 constraints | `src/instance_manager.cpp::set_active_cap` + `src/memory_monitor.cpp` |
| §9 kernel semantics | delegated to the real Pantograph repl |
| §10 API | `LeanFFI::run_file` / `run_source` in `src/leanffi.cpp` |
| §11 env | `instance_<id>/env/` |
| §12 GoalState | `src/leanffi.h::GoalState` |
| §13 tactic | `LeanFFI::tactic` in `src/leanffi.cpp` |
| §14 injection | `LeanFFI::add_theorem/lemma/definition/structure/class/instance` |
| §15 snapshot | `src/snapshot.cpp` |
| §16 manager | `src/instance_manager.cpp` |
| §17 scheduler | `src/scheduler.cpp` (ROUND_ROBIN/LEAST_LOAD/DAG_AWARE) |
| §18 validation | `src/validation.cpp` + `validation/main.cpp` |
| §19 cmake | `cmake/CMakeLists.txt` |
| §20 observability | logs + evidence only; no persistent dashboards |
| §21 pipeline | `run_pipeline.sh` |
| §22 success | evaluated by `lpi_validation --all` |

## Environment notes

The system targets the spec's abstract 10,000-instance scale. The
runtime active-cap is bounded by the host's memory (each repl uses
~150 MB of RSS). The `set_active_cap` knob is the single point of
control; the spec target is recorded in evidence and in the
`lpi_full_pipeline` CMake target.

## Hard constraints honoured

* **Pantograph is never modified**: we only `lake build` in it; the
  build script reads its source files and copies the produced binary
  out. We never `cp`, `mv`, or `write` into `/root/mycode/Pantograph`.
* **All runtime state stays in this workspace**: `runtime/`,
  `evidence/`, `evolution_logs/`, `snapshots/`, `forks/`.
* **All evidence is real**: every file under `evidence/` is written
  by code that produced the data; the `evidence/INDEX.json` is a
  catalogue, not a source of truth.
* **Git is audit-only**: `git add/commit/status` only; no
  `git log/diff/blame` is used for reasoning.
