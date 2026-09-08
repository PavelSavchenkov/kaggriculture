cmake_minimum_required(VERSION 3.18)
project(animal_service_cost LANGUAGES CXX)
get_filename_component(EXPERIMENT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
get_filename_component(REPOSITORY "${EXPERIMENT}/../../.." ABSOLUTE)
add_subdirectory("${REPOSITORY}/day_solver" day_solver)
add_library(agents OBJECT
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/league/public_router/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_003/parent_source/league/public_router/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_003/parent_source/league/top_replay_library/source/agent.cpp"
    "${REPOSITORY}/experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_004/proposals/observed_sale_lead_start_216/source/agent.cpp"
)
target_compile_features(agents PRIVATE cxx_std_20)
target_include_directories(agents PRIVATE "${REPOSITORY}")
target_compile_options(agents PRIVATE -O3 -march=native -mtune=native)
foreach(target care_only care_minimize)
    add_executable(${target} source/compile.cpp $<TARGET_OBJECTS:agents>)
    target_include_directories(${target} BEFORE PRIVATE "${REPOSITORY}")
    target_include_directories(${target} SYSTEM PRIVATE "${REPOSITORY}/day_solver/vendor/include")
    target_compile_definitions(${target} PRIVATE OR_PROTO_DLL= PROTOBUF_USE_DLLS)
    target_compile_options(${target} PRIVATE -O3 -march=native -mtune=native)
    target_link_libraries(${target} PRIVATE DaySolver::scheduler)
endforeach()
target_compile_definitions(care_only PRIVATE SERVICE_MIN_HIRES=0)
target_compile_definitions(care_minimize PRIVATE SERVICE_MIN_HIRES=-1)
