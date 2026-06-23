# LeanFFI Multi-Instance Evolution Engine — CMake Build
# Target: ≥ CMake 3.25 per requirements/performance/01_performance.md

cmake_minimum_required(VERSION 3.25)
project(leanffi_evolution CXX)

# Project layout: this CMakeLists is in cmake/; sources in ../runtime and ../scheduler.
set(EXT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Where Pantograph's REPL binary lives (built externally, read-only here).
set(PANTOGRAPH_REPL "/root/mycode/Pantograph/.lake/build/bin/repl"
    CACHE PATH "Path to Pantograph repl executable")

# Sources
set(LEANFFI_SRC
    ${EXT_DIR}/runtime/src/leanffi.cpp
    ${EXT_DIR}/runtime/src/instance_manager.cpp
)
set(SCHED_SRC
    ${EXT_DIR}/scheduler/src/scheduler.cpp
)

add_library(leanffi_core STATIC ${LEANFFI_SRC})
target_include_directories(leanffi_core PUBLIC
    ${EXT_DIR}/runtime/include
)

add_library(scheduler_core STATIC ${SCHED_SRC})
target_include_directories(scheduler_core PUBLIC
    ${EXT_DIR}/runtime/include
)
target_link_libraries(scheduler_core PUBLIC leanffi_core)

# === Runtime Targets ===

# Spawn the configured number of LeanFFI instances and run them.
add_executable(spawn_10000_instances
    ${EXT_DIR}/runtime/src/orchestrator.cpp
)
target_link_libraries(spawn_10000_instances PRIVATE
    leanffi_core scheduler_core
)
target_compile_options(spawn_10000_instances PRIVATE -Wall -Wextra -O2)

# Time-bounded simulation: alias to spawn_10000_instances.
add_executable(run_time_bounded_simulation
    ${EXT_DIR}/runtime/src/orchestrator.cpp
)
set_target_properties(run_time_bounded_simulation PROPERTIES OUTPUT_NAME "spawn_10000_instances")
target_link_libraries(run_time_bounded_simulation PRIVATE leanffi_core scheduler_core)
target_compile_options(run_time_bounded_simulation PRIVATE -Wall -Wextra -O2)

# Memory check: rss sampling around a run.
add_executable(memory_check
    ${EXT_DIR}/runtime/src/memory_check.cpp
)
target_link_libraries(memory_check PRIVATE leanffi_core)
target_compile_options(memory_check PRIVATE -Wall -Wextra -O2)

# Monitor deadline: alias to orchestrator.
add_executable(monitor_deadline
    ${EXT_DIR}/runtime/src/orchestrator.cpp
)
set_target_properties(monitor_deadline PROPERTIES OUTPUT_NAME "spawn_10000_instances")
target_link_libraries(monitor_deadline PRIVATE leanffi_core scheduler_core)
target_compile_options(monitor_deadline PRIVATE -Wall -Wextra -O2)

# === Validation Targets ===

add_executable(validate_semantic
    ${EXT_DIR}/runtime/src/validate_semantic.cpp
)
target_link_libraries(validate_semantic PRIVATE leanffi_core)
target_compile_options(validate_semantic PRIVATE -Wall -Wextra -O2)

add_executable(validate_isolation
    ${EXT_DIR}/runtime/src/validate_isolation.cpp
)
target_link_libraries(validate_isolation PRIVATE leanffi_core scheduler_core)
target_compile_options(validate_isolation PRIVATE -Wall -Wextra -O2)

add_executable(validate_requirements
    ${EXT_DIR}/runtime/src/validate_requirements.cpp
)
target_link_libraries(validate_requirements PRIVATE leanffi_core)
target_compile_options(validate_requirements PRIVATE -Wall -Wextra -O2)

add_executable(validate_all
    ${EXT_DIR}/runtime/src/validate_all.cpp
)
target_link_libraries(validate_all PRIVATE leanffi_core scheduler_core)
target_compile_options(validate_all PRIVATE -Wall -Wextra -O2)

# === Reporting Targets ===

add_executable(report_runtime
    ${EXT_DIR}/runtime/src/orchestrator.cpp
)
set_target_properties(report_runtime PROPERTIES OUTPUT_NAME "spawn_10000_instances")
target_link_libraries(report_runtime PRIVATE leanffi_core scheduler_core)
target_compile_options(report_runtime PRIVATE -Wall -Wextra -O2)

add_executable(report_memory
    ${EXT_DIR}/runtime/src/memory_check.cpp
)
set_target_properties(report_memory PROPERTIES OUTPUT_NAME "memory_check")
target_link_libraries(report_memory PRIVATE leanffi_core)
target_compile_options(report_memory PRIVATE -Wall -Wextra -O2)

add_executable(report_scaling
    ${EXT_DIR}/runtime/src/orchestrator.cpp
)
set_target_properties(report_scaling PROPERTIES OUTPUT_NAME "spawn_10000_instances")
target_link_libraries(report_scaling PRIVATE leanffi_core scheduler_core)
target_compile_options(report_scaling PRIVATE -Wall -Wextra -O2)

add_executable(report_all
    ${EXT_DIR}/runtime/src/orchestrator.cpp
)
set_target_properties(report_all PROPERTIES OUTPUT_NAME "spawn_10000_instances")
target_link_libraries(report_all PRIVATE leanffi_core scheduler_core)
target_compile_options(report_all PRIVATE -Wall -Wextra -O2)

# === Unit Tests (no subprocess required) ===
add_executable(unit_test
    ${EXT_DIR}/runtime/src/unit_test.cpp
)
target_link_libraries(unit_test PRIVATE leanffi_core scheduler_core)
target_compile_options(unit_test PRIVATE -Wall -Wextra -O2)
