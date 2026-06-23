# FFI Requirements

## FFI-001: Semantic Equivalence

LeanFFI MUST be semantically equivalent to `lean` CLI.
Formal: ∀P: LeanFFI(P) == lean(P)

Equivalence includes:
- parsing
- elaboration
- kernel checks
- errors
- warnings
- module resolution

Implementation: LeanFFI is a thin wrapper around the Pantograph REPL process.
Both `lean` and Pantograph REPL share Lean's front-end/elaborator/kernel.
Test: a source string accepted by `lean` must produce identical elaboration
results under LeanFFI; a source string rejected by `lean` must be rejected
identically by LeanFFI.

## FFI-002: Execution Interfaces

### File execution
```cpp
Result run_file(path)
```

### Source execution
```cpp
Result run_source(code)
```

## FFI-003: Evolution Interfaces

LeanFFI supports:
- load source string
- load file
- create goal
- remove goal
- add theorem
- add lemma
- add definition
- add structure
- add class
- add instance
- snapshot / restore / fork

All operations go through Pantograph REPL JSON protocol commands.

## FFI-004: Isolation Rule

No cross-instance leakage:
```
LeanFFI_A ≠ LeanFFI_B
```
Each instance is an independent OS process with independent Lean runtime state.

## FFI-005: Isolation Implementation

Each LeanFFI instance:
- spawns a dedicated `repl` subprocess
- holds unique stdin/stdout pipes
- has independent file-descriptor table
- has independent Lean environment memory
- has independent goal-state table

Inter-instance communication: NONE. Only the orchestrator (parent process)
may read collected results.