add_library(agent_sep23_day_core STATIC
    source/state.cpp source/history.cpp source/biology.cpp source/intent.cpp
    source/commitments.cpp source/trading.cpp source/workload.cpp
    source/remaining.cpp source/funding.cpp source/schedule.cpp
    source/seller.cpp source/allocation.cpp source/inventory.cpp)
add_library(agent_sep23_day_worker STATIC
    worker/source/agent.cpp worker/source/routes.cpp worker/source/policy.cpp
    worker/source/verify.cpp)
add_library(agent_sep23_day_compiler STATIC
    source/compiler.cpp source/reactive.cpp source/market.cpp source/forecast.cpp
    source/input_market.cpp source/rolling.cpp)

foreach(target agent_sep23_day_core agent_sep23_day_worker agent_sep23_day_compiler)
    target_include_directories(${target} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${KAGGRICULTURE_ROOT}")
    target_compile_options(${target} PRIVATE -Wall -Wextra -fno-exceptions -fno-rtti $<$<CONFIG:Release>:-O3>)
endforeach()
target_link_libraries(agent_sep23_day_compiler PUBLIC agent_sep23_day_core agent_sep23_day_worker)
add_library(day_compiler::release ALIAS agent_sep23_day_compiler)
