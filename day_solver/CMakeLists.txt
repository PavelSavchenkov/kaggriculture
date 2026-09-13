cmake_minimum_required(VERSION 3.18)
project(kaggriculture_day_solver VERSION 1.2.0 LANGUAGES CXX)
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

add_library(day_scheduler STATIC src/scheduler.cpp src/prepare_problem.cpp src/storage.cpp src/fast_routes.cpp src/repair_partition.cpp src/event_routes.cpp src/job_beam.cpp
    "${DAY_SOLVER_COMPONENTS}/native_quick_portfolio_v30/quick.cpp"
    "${DAY_SOLVER_COMPONENTS}/native_materialized_screen/screen.cpp"
    "${DAY_SOLVER_COMPONENTS}/native_solver_api/exact.cpp"
    "${DAY_SOLVER_COMPONENTS}/native_exact_domains/solver/src/tile_graph.cpp"
    "${DAY_SOLVER_COMPONENTS}/native_exact_domains/solver/src/problem_json.cpp"
    "${DAY_SOLVER_COMPONENTS}/native_exact_domains/solver/src/problem_validation.cpp")
add_library(DaySolver::scheduler ALIAS day_scheduler)
day_solver_compile(day_scheduler)
target_include_directories(day_scheduler PRIVATE
    "${DAY_SOLVER_COMPONENTS}/native_constructor_internal"
    "${DAY_SOLVER_COMPONENTS}/native_constructor_data"
    "${DAY_SOLVER_COMPONENTS}/native_constructor_repair"
    "${DAY_SOLVER_COMPONENTS}/native_cpp_sdk/pyvrp/PyVRP-0.14.0/pyvrp/cpp")
if(DAY_SOLVER_REBUILD_CORE)
    target_link_libraries(day_scheduler PUBLIC day_solver_core)
else()
    target_link_libraries(day_scheduler PUBLIC ${DAY_SOLVER_BUNDLED_LIBRARIES})
endif()
target_link_options(day_scheduler INTERFACE "-Wl,-rpath,${DAY_SOLVER_DIR}/runtime/lib")

add_executable(day_solver_cli src/solve_cli.cpp)
day_solver_compile(day_solver_cli)
find_path(DAY_SOLVER_OPENSSL_INCLUDE openssl/sha.h HINTS "$ENV{CONDA_PREFIX}/include" REQUIRED)
target_include_directories(day_solver_cli SYSTEM PRIVATE "${DAY_SOLVER_OPENSSL_INCLUDE}")
target_link_libraries(day_solver_cli PRIVATE day_scheduler)

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
    add_executable(test_constructor_budget tests/test_constructor_budget.cpp)
    day_solver_compile(test_constructor_budget)
    target_include_directories(test_constructor_budget PRIVATE
        "${DAY_SOLVER_COMPONENTS}/native_constructor_internal"
        "${DAY_SOLVER_COMPONENTS}/native_constructor_data"
        "${DAY_SOLVER_COMPONENTS}/native_constructor_repair"
        "${DAY_SOLVER_COMPONENTS}/native_cpp_sdk/pyvrp/PyVRP-0.14.0/pyvrp/cpp")
    target_link_libraries(test_constructor_budget PRIVATE DaySolver::scheduler)
    add_test(NAME day_solver_constructor_budget COMMAND "${DAY_SOLVER_DIR}/with_runtime.sh"
        "$<TARGET_FILE:test_constructor_budget>" "${DAY_SOLVER_DIR}/tests/fixtures/105433860_p1_d23.json"
        "${DAY_SOLVER_DIR}/tests/fixtures/synthetic_goose_egg_day.json")
    add_executable(test_materialized_stock tests/test_materialized_stock.cpp)
    day_solver_compile(test_materialized_stock)
    target_link_libraries(test_materialized_stock PRIVATE DaySolver::scheduler)
    add_test(NAME day_solver_materialized_stock COMMAND "${DAY_SOLVER_DIR}/with_runtime.sh"
        "$<TARGET_FILE:test_materialized_stock>"
        "${DAY_SOLVER_DIR}/tests/materialization/third_day15.json"
        "${DAY_SOLVER_DIR}/tests/materialization/third_day15.actions"
        "${DAY_SOLVER_DIR}/tests/materialization/third_day28.json"
        "${DAY_SOLVER_DIR}/tests/materialization/third_day28.actions"
        "${DAY_SOLVER_DIR}/tests/materialization/unknown_mother_day25.json"
        "${DAY_SOLVER_DIR}/tests/materialization/unknown_mother_day25.actions")
    add_executable(test_day_storage tests/test_storage.cpp)
    day_solver_compile(test_day_storage)
    target_link_libraries(test_day_storage PRIVATE DaySolver::scheduler)
    add_test(NAME day_solver_storage COMMAND "${DAY_SOLVER_DIR}/with_runtime.sh"
        "$<TARGET_FILE:test_day_storage>" "${DAY_SOLVER_DIR}/tests/storage"
        "$<TARGET_FILE_DIR:test_day_storage>/storage_regression.actions")
    add_executable(test_day_scheduler tests/test_scheduler.cpp)
    target_link_libraries(test_day_scheduler PRIVATE DaySolver::scheduler)
    add_executable(test_cold_day tests/test_cold_day.cpp)
    target_link_libraries(test_cold_day PRIVATE DaySolver::scheduler)
    add_test(NAME day_solver_cold COMMAND "${DAY_SOLVER_DIR}/with_runtime.sh"
        "$<TARGET_FILE:test_cold_day>"
        "${DAY_SOLVER_DIR}/tests/cold/early_milk_handoff.json"
        "${DAY_SOLVER_DIR}/tests/cold/staged_seed_and_animal_purchases.json"
        "${DAY_SOLVER_DIR}/tests/cold/unpolished_completion.json"
        "${DAY_SOLVER_DIR}/tests/cold/third_day14.json"
        "${DAY_SOLVER_DIR}/tests/cold/regret_third_day15.json"
        "${DAY_SOLVER_DIR}/tests/cold/rebuilt_unknown_day25.json"
        "${DAY_SOLVER_DIR}/tests/cold/early_fertilizer_late_wheat.json")
    add_test(NAME day_solver_search_regressions COMMAND "${DAY_SOLVER_DIR}/with_runtime.sh"
        "$<TARGET_FILE:test_cold_day>" --search-regressions
        "${DAY_SOLVER_DIR}/tests/cold/local_wheat_sale.json"
        "${DAY_SOLVER_DIR}/tests/cold/idle_last_hire.json")
    file(GLOB fixtures "${DAY_SOLVER_DIR}/tests/fixtures/*.json")
    add_test(NAME day_solver_api COMMAND "${DAY_SOLVER_DIR}/with_runtime.sh"
        "$<TARGET_FILE:test_day_scheduler>" ${fixtures})
    add_executable(test_problem_builder tests/test_problem_builder.cpp)
    target_link_libraries(test_problem_builder PRIVATE DaySolver::scheduler)
    add_test(NAME day_solver_problem_builder COMMAND "${DAY_SOLVER_DIR}/with_runtime.sh"
        "$<TARGET_FILE:test_problem_builder>" ${fixtures})
endif()
