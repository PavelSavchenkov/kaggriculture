cmake_minimum_required(VERSION 3.18)
project(animal_forecast_error LANGUAGES CXX)
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

add_executable(mixed source/mixed.cpp
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/league/public_router/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_003/parent_source/league/public_router/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_003/parent_source/league/top_replay_library/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_004/proposals/observed_sale_lead_start_216/source/agent.cpp"
)
target_include_directories(mixed BEFORE PRIVATE "${REPOSITORY}")
target_include_directories(mixed SYSTEM PRIVATE "${REPOSITORY}/day_solver/vendor/include")
target_compile_definitions(mixed PRIVATE OR_PROTO_DLL= PROTOBUF_USE_DLLS)
target_compile_options(mixed PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(mixed PRIVATE DaySolver::scheduler)

add_executable(mixed_certificate source/mixed_certificate.cpp
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/league/public_router/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_003/parent_source/league/public_router/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_003/parent_source/league/top_replay_library/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_004/proposals/observed_sale_lead_start_216/source/agent.cpp"
)
target_include_directories(mixed_certificate BEFORE PRIVATE "${REPOSITORY}")
target_include_directories(mixed_certificate SYSTEM PRIVATE "${REPOSITORY}/day_solver/vendor/include")
target_compile_definitions(mixed_certificate PRIVATE OR_PROTO_DLL= PROTOBUF_USE_DLLS)
target_compile_options(mixed_certificate PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(mixed_certificate PRIVATE DaySolver::scheduler)
