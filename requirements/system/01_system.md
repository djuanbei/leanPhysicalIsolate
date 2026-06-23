# System Requirements

## SYS-001: Scale

Target:
- 10,000 LeanFFI instances (logical, with bounded physical concurrency)
- 10 Lean files × 10,000 instances = 100,000 evaluations (logical)

## SYS-002: Time Constraint

Total runtime: < 5 hours
Minimum throughput: ≥ 6 eval/sec

## SYS-003: Memory Constraint

M(t) ≤ M(0) + ε
No accumulation allowed. Process exits and reloads as needed.

## SYS-004: Isolation Constraint

Each LeanFFI instance:
- independent OS process
- independent Lean runtime
- no shared state

## SYS-005: Physical Concurrency Bound

Physical concurrency MUST be bounded by host capability:
- C_phys ≤ max(1, floor((mem_available_MB - 256) / per_instance_MB))
- C_phys ≤ nproc * factor (factor = 2 typical)
- C_phys enforced as cap; logical instances > C_phys are time-shared.

This constraint allows honest execution on resource-limited hosts
while preserving the 10,000-instance logical specification.