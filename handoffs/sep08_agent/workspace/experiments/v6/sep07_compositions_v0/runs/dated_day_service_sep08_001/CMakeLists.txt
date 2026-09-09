cmake_minimum_required(VERSION 3.18)
project(dated_day_service LANGUAGES CXX)
get_filename_component(EXPERIMENT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
get_filename_component(REPOSITORY "${EXPERIMENT}/../../.." ABSOLUTE)
add_subdirectory("${REPOSITORY}/day_solver" day_solver)
add_executable(compile_days compile.cpp
    "${EXPERIMENT}/runs/compiler_care_sep08_001/compiler/source/agent.cpp"
    "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
    "${EXPERIMENT}/league/public_router/source/agent.cpp")
target_include_directories(compile_days BEFORE PRIVATE "${REPOSITORY}")
target_compile_options(compile_days PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(compile_days PRIVATE DaySolver::scheduler)
