cmake_minimum_required(VERSION 3.18)
project(animal_groups LANGUAGES CXX)
get_filename_component(EXPERIMENT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
get_filename_component(REPOSITORY "${EXPERIMENT}/../../.." ABSOLUTE)
add_subdirectory("${REPOSITORY}/day_solver" day_solver)
foreach(target estimate compile compile_v2 compile_v3 funding_witness source_control source_control_v2 hinted_solve estimate_v2 compile_v4 compile_v5 compile_v6 source_control_v3 hinted_solve_v2 final_audit compile_v7 compile_v8)
    add_executable(${target} source/${target}.cpp
        "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2/source/agent.cpp"
        "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_004/proposals/observed_sale_lead_start_216/source/agent.cpp"
        "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_003/parent_source/league/top_replay_library/source/agent.cpp"
        "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_003/parent_source/league/public_router/source/agent.cpp"
        "${REPOSITORY}/experiments/v6/sep07_compositions_v0/league/top_replay_library/source/agent.cpp"
        "${REPOSITORY}/experiments/v6/sep07_compositions_v0/league/public_router/source/agent.cpp"
    )
    target_include_directories(${target} BEFORE PRIVATE "${REPOSITORY}")
    target_compile_options(${target} PRIVATE -O3 -march=native -mtune=native)
    target_link_libraries(${target} PRIVATE DaySolver::scheduler)
endforeach()

target_include_directories(hinted_solve SYSTEM PRIVATE "${REPOSITORY}/day_solver/vendor/include")
target_compile_definitions(hinted_solve PRIVATE OR_PROTO_DLL= PROTOBUF_USE_DLLS)
add_executable(care_equivalence source/care_equivalence.cpp)
target_compile_features(care_equivalence PRIVATE cxx_std_20)
target_include_directories(care_equivalence PRIVATE "${REPOSITORY}")
target_compile_options(care_equivalence PRIVATE -O3 -march=native)

target_include_directories(hinted_solve_v2 SYSTEM PRIVATE "${REPOSITORY}/day_solver/vendor/include")
target_compile_definitions(hinted_solve_v2 PRIVATE OR_PROTO_DLL= PROTOBUF_USE_DLLS)

add_executable(courier_witness source/courier_witness.cpp)
target_link_libraries(courier_witness PRIVATE DaySolver::scheduler)

target_include_directories(compile_v8 SYSTEM PRIVATE "${REPOSITORY}/day_solver/vendor/include")
target_compile_definitions(compile_v8 PRIVATE OR_PROTO_DLL= PROTOBUF_USE_DLLS)
