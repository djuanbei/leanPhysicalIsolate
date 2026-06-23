# Performance Requirements

## PERF-001: Throughput

Minimum throughput: ≥ 6 eval/sec (averaged over full run).

## PERF-002: Latency

Per-evaluation wall-time: P95 ≤ 30s (relaxable on resource-constrained hosts).

## PERF-003: Resource Efficiency

- Memory per physical LeanFFI instance ≤ 200MB (typical Lean REPL footprint)
- CPU utilization ≤ 95% sustained

## PERF-004: Scalability

Performance MUST scale sub-linearly with logical instance count.
Resource-bounded execution model preserves the 10,000 logical-instance
specification by time-sharing physical instances.

## PERF-005: Reporting

All metrics reported from real measurements; no synthetic or mock data.