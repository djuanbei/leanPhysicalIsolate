cmake_minimum_required(VERSION 3.16)
project(lean_physical_isolate CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# -----------------------------------------------------------------------------
# Source layout
# -----------------------------------------------------------------------------
set(LPI_SOURCES
    ${CMAKE_SOURCE_DIR}/../src/result.cpp
    ${CMAKE_SOURCE_DIR}/../src/logger.cpp
    ${CMAKE_SOURCE_DIR}/../src/instance_manager.cpp
    ${CMAKE_SOURCE_DIR}/../src/scheduler.cpp
    ${CMAKE_SOURCE_DIR}/../src/leanffi.cpp
    ${CMAKE_SOURCE_DIR}/../src/goal_state.cpp
    ${CMAKE_SOURCE_DIR}/../src/snapshot.cpp
    ${CMAKE_SOURCE_DIR}/../src/validation.cpp
    ${CMAKE_SOURCE_DIR}/../src/requirements.cpp
    ${CMAKE_SOURCE_DIR}/../src/evidence.cpp
    ${CMAKE_SOURCE_DIR}/../src/memory_monitor.cpp
    ${CMAKE_SOURCE_DIR}/../src/pantograph_protocol.cpp
    ${CMAKE_SOURCE_DIR}/../src/json_helpers.cpp
)

add_library(lpi_core STATIC ${LPI_SOURCES})
target_include_directories(lpi_core PUBLIC ${CMAKE_SOURCE_DIR}/../src)
target_compile_options(lpi_core PRIVATE -Wall -Wextra -O2)

# -----------------------------------------------------------------------------
# Executables
# -----------------------------------------------------------------------------
add_executable(lpi_instance_manager
    ${CMAKE_SOURCE_DIR}/../instance_manager/main.cpp
)
target_link_libraries(lpi_instance_manager PRIVATE lpi_core)

add_executable(lpi_scheduler
    ${CMAKE_SOURCE_DIR}/../scheduler/main.cpp
)
target_link_libraries(lpi_scheduler PRIVATE lpi_core)

add_executable(lpi_leanffi
    ${CMAKE_SOURCE_DIR}/../leanffi/main.cpp
)
target_link_libraries(lpi_leanffi PRIVATE lpi_core)

add_executable(lpi_validation
    ${CMAKE_SOURCE_DIR}/../validation/main.cpp
)
target_link_libraries(lpi_validation PRIVATE lpi_core)

add_executable(lpi_evidence
    ${CMAKE_SOURCE_DIR}/../evidence_main/main.cpp
)
target_link_libraries(lpi_evidence PRIVATE lpi_core)

add_executable(lpi_requirements
    ${CMAKE_SOURCE_DIR}/../requirements_main/main.cpp
)
target_link_libraries(lpi_requirements PRIVATE lpi_core)

# -----------------------------------------------------------------------------
# Spec targets (CMake orchestration per spec #19)
# -----------------------------------------------------------------------------
set(LPI_BIN_DIR ${CMAKE_BINARY_DIR})
set(LPI_WORKSPACE ${CMAKE_SOURCE_DIR}/..)

# spawn_10000_instances
add_custom_target(spawn_10000_instances
    COMMAND ${LPI_BIN_DIR}/lpi_instance_manager --repl ${LPI_WORKSPACE}/repl --target 10000 --active-cap 64 --spawn-all
    WORKING_DIRECTORY ${LPI_WORKSPACE}
    COMMENT "Spawning isolated LeanFFI instances"
    USES_TERMINAL
)

# run_parallel_execution
add_custom_target(run_parallel_execution
    COMMAND ${LPI_BIN_DIR}/lpi_scheduler --repl ${LPI_WORKSPACE}/repl --target 10000 --active-cap 64 --policy ROUND_ROBIN --dispatch-all --workers 8
    WORKING_DIRECTORY ${LPI_WORKSPACE}
    COMMENT "Running parallel execution across all instances"
    USES_TERMINAL
)

# validate_all
add_custom_target(validate_all
    COMMAND ${LPI_BIN_DIR}/lpi_validation --repl ${LPI_WORKSPACE}/repl --all
    WORKING_DIRECTORY ${LPI_WORKSPACE}
    COMMENT "Running validation framework on all instances"
    USES_TERMINAL
)

# benchmark_all
add_custom_target(benchmark_all
    COMMAND ${LPI_BIN_DIR}/lpi_validation --repl ${LPI_WORKSPACE}/repl --benchmark
    WORKING_DIRECTORY ${LPI_WORKSPACE}
    COMMENT "Running benchmark suite"
    USES_TERMINAL
)

# memory_check
add_custom_target(memory_check
    COMMAND ${LPI_BIN_DIR}/lpi_validation --repl ${LPI_WORKSPACE}/repl --memory
    WORKING_DIRECTORY ${LPI_WORKSPACE}
    COMMENT "Verifying bounded-memory property M_active(t) <= M0 + eps"
    USES_TERMINAL
)

# requirements_evaluate
add_custom_target(requirements_evaluate
    COMMAND ${LPI_BIN_DIR}/lpi_requirements
    WORKING_DIRECTORY ${LPI_WORKSPACE}
    COMMENT "Evaluating all spec requirements and writing evidence/requirements.jsonl"
    USES_TERMINAL
)

# aggregate CI target
add_custom_target(lpi_full_pipeline
    DEPENDS spawn_10000_instances
    COMMAND ${CMAKE_COMMAND} -E echo "=== pipeline: instances spawned ==="
    COMMAND ${LPI_BIN_DIR}/lpi_scheduler --repl ${LPI_WORKSPACE}/repl --target 10000 --active-cap 64 --policy ROUND_ROBIN --dispatch-all --workers 8
    COMMAND ${CMAKE_COMMAND} -E echo "=== pipeline: execution done ==="
    COMMAND ${LPI_BIN_DIR}/lpi_validation --repl ${LPI_WORKSPACE}/repl --all
    COMMAND ${CMAKE_COMMAND} -E echo "=== pipeline: validation done ==="
    COMMAND ${LPI_BIN_DIR}/lpi_validation --repl ${LPI_WORKSPACE}/repl --memory
    COMMAND ${LPI_BIN_DIR}/lpi_validation --repl ${LPI_WORKSPACE}/repl --benchmark
    COMMAND ${CMAKE_COMMAND} -E echo "=== pipeline: benchmarks done ==="
    WORKING_DIRECTORY ${LPI_WORKSPACE}
    COMMENT "Full evolution pipeline: spawn -> execute -> validate -> memory -> benchmark"
    USES_TERMINAL
)
