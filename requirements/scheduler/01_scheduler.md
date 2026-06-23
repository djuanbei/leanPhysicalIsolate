# Scheduler Requirements

## SCH-001: Policies

Scheduler supports four policies:
- ROUND_ROBIN — cyclic assignment of files to instances
- LEAST_LOAD — assign to instance with fewest pending tasks
- AFFINITY — pin each file to a single instance (cache locality)
- DAG_AWARE — topological order respecting dependencies

## SCH-002: Stateless or Bounded State Only

Scheduler MUST NOT accumulate unbounded state.
- All task metadata is per-task; no long-term history storage.
- Stream results; do not retain completed task payloads.

## SCH-003: Streaming Execution Only

Tasks are dispatched and consumed in a streaming pipeline.
Backpressure: when consumer is slow, scheduler pauses dispatch.

## SCH-004: No Accumulation

Memory budget M(t) ≤ M(0) + ε for all t.