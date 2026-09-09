cmake_minimum_required(VERSION 3.18)
project(mixed_funding LANGUAGES CXX)
get_filename_component(EXPERIMENT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
get_filename_component(REPOSITORY "${EXPERIMENT}/../../.." ABSOLUTE)
add_subdirectory("${REPOSITORY}/day_solver" day_solver)
add_executable(diagnostic source/diagnostic.cpp
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/league/public_router/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_003/parent_source/league/public_router/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_003/parent_source/league/top_replay_library/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_004/proposals/observed_sale_lead_start_216/source/agent.cpp"
)
target_include_directories(diagnostic BEFORE PRIVATE "${REPOSITORY}")
target_include_directories(diagnostic SYSTEM PRIVATE "${REPOSITORY}/day_solver/vendor/include")
target_compile_definitions(diagnostic PRIVATE OR_PROTO_DLL= PROTOBUF_USE_DLLS)
target_compile_options(diagnostic PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(diagnostic PRIVATE DaySolver::scheduler)

add_executable(forecast source/forecast.cpp
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/league/public_router/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_003/parent_source/league/public_router/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_003/parent_source/league/top_replay_library/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_004/proposals/observed_sale_lead_start_216/source/agent.cpp"
)
target_include_directories(forecast BEFORE PRIVATE "${REPOSITORY}")
target_include_directories(forecast SYSTEM PRIVATE "${REPOSITORY}/day_solver/vendor/include")
target_compile_definitions(forecast PRIVATE OR_PROTO_DLL= PROTOBUF_USE_DLLS)
target_compile_options(forecast PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(forecast PRIVATE DaySolver::scheduler)

add_executable(forecast_v2 source/forecast_v2.cpp
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/league/public_router/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_003/parent_source/league/public_router/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_003/parent_source/league/top_replay_library/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_004/proposals/observed_sale_lead_start_216/source/agent.cpp"
)
target_include_directories(forecast_v2 BEFORE PRIVATE "${REPOSITORY}")
target_include_directories(forecast_v2 SYSTEM PRIVATE "${REPOSITORY}/day_solver/vendor/include")
target_compile_definitions(forecast_v2 PRIVATE OR_PROTO_DLL= PROTOBUF_USE_DLLS)
target_compile_options(forecast_v2 PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(forecast_v2 PRIVATE DaySolver::scheduler)
