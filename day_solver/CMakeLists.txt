cmake_minimum_required(VERSION 3.18)
project(kaggriculture_day_solver VERSION 1.0.0 LANGUAGES CXX)
set(DAY_SOLVER_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
    message(FATAL_ERROR "The bundled V30 binaries require Linux x86-64; see docs/integration.md")
endif()
option(DAY_SOLVER_BUILD_TESTS "Build exposed regression controls" OFF)
option(DAY_SOLVER_BUILD_EXAMPLES "Build the C++ usage example" OFF)
option(DAY_SOLVER_REBUILD_CORE "Recompile portfolio/models; retain bundled routing and numeric libraries" OFF)
include(cmake/bundled_libraries.cmake)
set(DAY_SOLVER_COMPONENTS "${DAY_SOLVER_DIR}/src/components")
set(DAY_SOLVER_API "${DAY_SOLVER_COMPONENTS}/resource_portfolio_order_v2/solver/include")

function(day_solver_compile target)
    target_compile_features(${target} PUBLIC cxx_std_20)
    target_compile_options(${target} PRIVATE -O3 -fwrapv -Wall -Wextra -Wpedantic)
    target_compile_definitions(${target} PRIVATE NDEBUG OR_PROTO_DLL= PROTOBUF_USE_DLLS
        EIGEN_MPL2_ONLY HAVE_CONFIG_H USE_BOP USE_CBC USE_CLP USE_GLOP USE_HIGHS USE_MATH_OPT USE_PDLP USE_SCIP)
    target_include_directories(${target} PUBLIC "${DAY_SOLVER_DIR}/include" "${DAY_SOLVER_API}")
    target_include_directories(${target} SYSTEM PRIVATE "${DAY_SOLVER_DIR}/vendor/include")
endfunction()

if(DAY_SOLVER_REBUILD_CORE)
    include(cmake/core_sources.cmake)
    add_library(day_solver_core STATIC ${DAY_SOLVER_CORE_SOURCES})
    day_solver_compile(day_solver_core)
    target_include_directories(day_solver_core PRIVATE
        "${DAY_SOLVER_COMPONENTS}/native_constructor_internal"
        "${DAY_SOLVER_COMPONENTS}/native_constructor_data"
        "${DAY_SOLVER_COMPONENTS}/native_constructor_repair"
        "${DAY_SOLVER_COMPONENTS}/native_cpp_sdk/pyvrp/PyVRP-0.14.0/pyvrp/cpp")
    set(core_dependencies ${DAY_SOLVER_BUNDLED_LIBRARIES})
    list(FILTER core_dependencies EXCLUDE REGEX "/lib/libday_.*\\.a$")
    target_link_libraries(day_solver_core PUBLIC ${core_dependencies})
endif()

add_library(day_scheduler STATIC src/scheduler.cpp src/prepare_problem.cpp)
add_library(DaySolver::scheduler ALIAS day_scheduler)
day_solver_compile(day_scheduler)
if(DAY_SOLVER_REBUILD_CORE)
    target_link_libraries(day_scheduler PUBLIC day_solver_core)
else()
    target_link_libraries(day_scheduler PUBLIC ${DAY_SOLVER_BUNDLED_LIBRARIES})
endif()
target_link_options(day_scheduler INTERFACE "-Wl,-rpath,${DAY_SOLVER_DIR}/runtime/lib")

add_executable(day_solver_audit EXCLUDE_FROM_ALL
    src/components/native_exact_domains/solver/src/audit_baseline.cpp)
day_solver_compile(day_solver_audit)
target_link_libraries(day_solver_audit PRIVATE day_scheduler)

if(DAY_SOLVER_BUILD_EXAMPLES)
    add_executable(day_solver_example examples/cpp/main.cpp)
    target_link_libraries(day_solver_example PRIVATE DaySolver::scheduler)
endif()
if(DAY_SOLVER_BUILD_TESTS)
    enable_testing()
    add_executable(test_day_scheduler tests/test_scheduler.cpp)
    target_link_libraries(test_day_scheduler PRIVATE DaySolver::scheduler)
    file(GLOB fixtures "${DAY_SOLVER_DIR}/tests/fixtures/*.json")
    add_test(NAME day_solver_api COMMAND "${DAY_SOLVER_DIR}/with_runtime.sh"
        "$<TARGET_FILE:test_day_scheduler>" ${fixtures})
    add_executable(test_problem_builder tests/test_problem_builder.cpp)
    target_link_libraries(test_problem_builder PRIVATE DaySolver::scheduler)
    add_test(NAME day_solver_problem_builder COMMAND "${DAY_SOLVER_DIR}/with_runtime.sh"
        "$<TARGET_FILE:test_problem_builder>" ${fixtures})
endif()
