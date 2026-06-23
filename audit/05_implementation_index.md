# Phase 5: Implementation Index

This is the source-of-truth pointer to the implementation. All code lives
externally at /Pantograph.ext/ to satisfy the source-immutability rule
(/root/mycode/Pantograph is read-only).

External layout:
- /Pantograph.ext/runtime/include/  - public headers (leanffi.hpp, scheduler.hpp, instance_manager.hpp)
- /Pantograph.ext/runtime/src/      - C++ implementations + main orchestrator
- /Pantograph.ext/scheduler/src/    - scheduler C++ source
- /Pantograph.ext/cmake/            - CMakeLists.txt and shell scripts
- /Pantograph.ext/generated/lean_samples/ - 10 sample Lean files for evaluations
- /Pantograph.ext/builds/           - build output (excluded from git)
- /Pantograph.ext/reports/          - validation reports
- /Pantograph.ext/evolution_logs/   - audit log
- /Pantograph.ext/requirements/     - requirement specs

Git-tracked mirrors (for audit ledger only; canonical source at /Pantograph.ext/):
- code_runtime/  -> mirror of runtime/ contents at the time of commit
- code_scheduler/ -> mirror of scheduler/ contents
- code_cmake/    -> mirror of cmake/ contents (CMakeLists.txt + scripts)
- audit/         -> this directory

Re-derive mirrors: cp -r /Pantograph.ext/runtime /root/mycode/lean_physical_isolate/code_runtime
