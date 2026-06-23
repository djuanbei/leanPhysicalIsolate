# Phase 6: Runtime Strategy

## Backend Modes

The LeanFFI class supports two execution backends, both semantically
equivalent to `lean` CLI (they use the same Lean parser, elaborator, kernel):

### Mode 1: REPL (Pantograph subprocess)

Each LeanFFI instance is a dedicated Pantograph REPL subprocess driven
over its native JSON-line protocol via `frontend.process` command.

- Pros: persistent process, amortizes startup cost, exposes all 28 REPL
  commands (goal.*, expr.*, frontend.*, env.*).
- Cons: requires Pantograph to be built. The build is heavy on memory-
  constrained hosts.

### Mode 2: CLI (`lean` invocation per task)

Each task writes a temp `.lean` file and invokes `lean` on it. Each call
spawns a fresh `lean` process.

- Pros: zero build overhead; uses the Lean binary already on PATH.
- Cons: process startup cost per task; no persistent state.

## Mode Selection

- Default for production: REPL (uses Pantograph's full feature set).
- Default for resource-constrained environments: CLI (always available).
- Selectable via `--backend repl` or `--backend cli` on the orchestrator CLI.

## Why Both Are Semantically Equivalent

Both backends drive the same Lean toolchain (v4.29.1 per `lean-toolchain`).
The parser, elaborator, and kernel are the same code. A source string
accepted by one is accepted identically by the other, modulo message
formatting (the CLI prints messages to stderr; the REPL returns them as
JSON).
