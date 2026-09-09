cmake_minimum_required(VERSION 3.18)
project(v52_family LANGUAGES CXX)
get_filename_component(EXPERIMENT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
get_filename_component(REPOSITORY "${EXPERIMENT}/../../.." ABSOLUTE)
add_subdirectory("${REPOSITORY}/day_solver" day_solver)
add_executable(rebuild rebuild.cpp
    "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
    "${EXPERIMENT}/league/public_router/source/agent.cpp"
    "${EXPERIMENT}/league/public_router_v52/source/agent.cpp")
target_include_directories(rebuild BEFORE PRIVATE "${REPOSITORY}")
target_compile_options(rebuild PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(rebuild PRIVATE DaySolver::scheduler)
