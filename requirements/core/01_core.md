# Core Requirements

## CORE-001: Source Immutability

Pantograph source tree at `/root/mycode/Pantograph` MUST NOT be modified.
- No file writes to source tree
- No patches, rewrites, or code injection
- Only read, analyze, parse, index, build-against operations allowed
- Invariant: Pantograph(t) == Pantograph(0) for all t

## CORE-002: External Evolution Layer

All system evolution MUST occur at `/Pantograph.ext/`:
- runtime/ — LeanFFI wrappers, instance lifecycle
- scheduler/ — dispatch policies
- cmake/ — CMake build configuration
- generated/ — generated artifacts (read-only after generation)
- cache/ — runtime cache (bounded)
- temp/ — transient files (cleaned on shutdown)
- builds/ — build artifacts (excluded from git)
- reports/ — final validation reports
- evolution_logs/ — audit logs (timestamped, append-only)
- requirements/ — versioned requirement specs

## CORE-003: Evidence-Driven Development

No change without evidence. Forbidden:
- mock data
- synthetic grammar
- invented AST nodes
- fake metrics

Every change MUST cite evidence in its log entry.

## CORE-004: Traceability Chain

Evidence → Gap → Design → Implementation → Validation → Log → Git Commit

## CORE-005: No Orphan Rule

Forbidden:
- unlogged changes
- unvalidated changes
- uncommitted changes
- evidence-less changes