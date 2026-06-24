# Pantograph-backed LeanFFI Orchestrator

A physically isolated, evidence-driven, Git-audited Lean kernel–semantically equivalent execution engine built on an immutable Pantograph base.

## Architecture

- **Immutable base**: `/root/mycode/Pantograph` is read-only.
- **LeanFFI** is a thin orchestration layer: `LeanFFI(x) = Compose(Pantograph(x))`.
- **Workspace**: `/root/mycode/lean_physical_isolate` — all evolution occurs here.
- **Isolation**: each of up to 10,000 instances gets its own `runtime/instance_<id>/{env,goals,logs,cache,snapshots}/` subtree.
- **Scheduler policies**: `ROUND_ROBIN`, `LEAST_LOAD`, `DAG_AWARE`.
- **Corpus sampling**: deterministic seed; random `.lean` files from `/root/mycode/lean4`.
- **addTheorem / addLemma synthesis**: extracts real `theorem`/`lemma` declarations and injects them via Pantograph's `env.add` JSON RPC.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Targets (per spec §19)

```bash
cmake --build build --target spawn_10000_instances
cmake --build build --target run_parallel_execution
cmake --build build --target validate_all
cmake --build build --target benchmark_all
cmake --build build --target memory_check
cmake --build build --target pipeline_full
```

## Direct invocation

```bash
build/leanffi_orchestrator run --instances 10000 --evals 100000 --policy LEAST_LOAD
```

## Outputs

- `evidence/test_sampling/*.json` — random Lean corpus execution evidence
- `evidence/ffi_generated/*.json` — addTheorem/addLemma generation evidence
- `evidence/validation/*.json` — validation framework reports
- `evidence/snapshot/*.json` — snapshot metadata
- `evidence/runtime/summary.json` — per-instance aggregates
- `evolution_logs/events_<session>.jsonl` — structured event log
- `reports/audit_<session>.json` — final audit report