cmake_minimum_required(VERSION 3.20)
project(leanffi_orchestrator
    VERSION 4.0.0
    DESCRIPTION "Pantograph-backed physically isolated LeanFFI engine"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ----- Inputs from configuration -----
set(LEANFFI_WORK_ROOT "/root/mycode/lean_physical_isolate" CACHE PATH "Workspace root")
set(PANTOGRAPH_ROOT "/root/mycode/Pantograph" CACHE PATH "Immutable Pantograph root")
set(LEAN4_CORPUS_ROOT "/root/mycode/lean4" CACHE PATH "Lean4 corpus root")

# Where the Lean REPL is expected to live (immutable, produced by Pantograph).
set(LEANFFI_REPL_BIN "${PANTOGRAPH_ROOT}/.lake/build/bin/repl" CACHE FILEPATH "Pantograph REPL binary")

if(NOT EXISTS "${LEANFFI_REPL_BIN}")
    message(WARNING "Pantograph REPL not yet built at ${LEANFFI_REPL_BIN}; run `lake build` in ${PANTOGRAPH_ROOT}")
endif()

add_compile_options(-Wall -Wextra -Wno-unused-parameter -O3)

# ----- LeanFFI core library -----
set(LEANFFI_CORE_SOURCES
    src/json_min.cpp
    src/util.cpp
    src/logger.cpp
    src/evidence.cpp
    src/pantograph_bridge.cpp
    src/instance_manager.cpp
    src/scheduler.cpp
    src/corpus_sampler.cpp
    src/ffi_generator.cpp
    src/validation.cpp
    src/orchestrator.cpp
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(leanffi_core STATIC ${LEANFFI_CORE_SOURCES})
target_include_directories(leanffi_core PUBLIC src)
target_link_libraries(leanffi_core PUBLIC Threads::Threads)

# ----- Orchestrator executable -----
add_executable(leanffi_orchestrator src/main.cpp)
target_link_libraries(leanffi_orchestrator PRIVATE leanffi_core)

# Convenience for out-of-tree callers
set(LEANFFI_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src" CACHE INTERNAL "")
set(LEANFFI_CORE_LIB leanffi_core CACHE INTERNAL "")

# ----- Targets exposed per spec section 19 -----
add_custom_target(spawn_10000_instances
    COMMAND $<TARGET_FILE:leanffi_orchestrator> run --instances 10000 --policy LEAST_LOAD
    WORKING_DIRECTORY ${LEANFFI_WORK_ROOT}
    COMMENT "Spawning 10,000 isolated LeanFFI instances (bounded active REPLs)")

add_custom_target(run_parallel_execution
    COMMAND $<TARGET_FILE:leanffi_orchestrator> run --instances 10000 --evals 100000 --policy LEAST_LOAD
    WORKING_DIRECTORY ${LEANFFI_WORK_ROOT}
    COMMENT "Running parallel LeanFFI execution")

add_custom_target(validate_all
    COMMAND $<TARGET_FILE:leanffi_orchestrator> validate
    WORKING_DIRECTORY ${LEANFFI_WORK_ROOT}
    COMMENT "Running full validation suite")

add_custom_target(benchmark_all
    COMMAND $<TARGET_FILE:leanffi_orchestrator> benchmark
    WORKING_DIRECTORY ${LEANFFI_WORK_ROOT}
    COMMENT "Running benchmark suite")

add_custom_target(memory_check
    COMMAND $<TARGET_FILE:leanffi_orchestrator> memory-check
    WORKING_DIRECTORY ${LEANFFI_WORK_ROOT}
    COMMENT "Printing active memory baseline")

# Combined "everything" pipeline target
add_custom_target(pipeline_full
    DEPENDS spawn_10000_instances run_parallel_execution validate_all benchmark_all memory_check
    COMMENT "Full Pantograph-backed LeanFFI pipeline")

enable_testing()
add_test(NAME validate_all COMMAND $<TARGET_FILE:leanffi_orchestrator> validate
         WORKING_DIRECTORY ${LEANFFI_WORK_ROOT})

install(TARGETS leanffi_orchestrator leanffi_core
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)