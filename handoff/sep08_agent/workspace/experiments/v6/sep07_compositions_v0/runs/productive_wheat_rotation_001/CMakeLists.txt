cmake_minimum_required(VERSION 3.18)
project(productive_wheat_rotation LANGUAGES CXX)
get_filename_component(REPOSITORY "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)
set(EXPERIMENT "${REPOSITORY}/experiments/v6/sep07_compositions_v0")
add_subdirectory("${REPOSITORY}/day_solver" day_solver)
add_executable(season_dump source/season_dump.cpp
    "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
    "${EXPERIMENT}/league/public_router/source/agent.cpp")
target_include_directories(season_dump BEFORE PRIVATE "${REPOSITORY}")
target_compile_options(season_dump PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(season_dump PRIVATE DaySolver::scheduler)
add_executable(compile_wheat source/compile_wheat.cpp
    "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
    "${EXPERIMENT}/league/public_router/source/agent.cpp")
target_include_directories(compile_wheat BEFORE PRIVATE "${REPOSITORY}")
target_compile_options(compile_wheat PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(compile_wheat PRIVATE DaySolver::scheduler)
add_executable(compile_wheat_berry source/compile_wheat.cpp
    "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
    "${EXPERIMENT}/league/public_router/source/agent.cpp")
target_include_directories(compile_wheat_berry BEFORE PRIVATE "${REPOSITORY}")
target_compile_definitions(compile_wheat_berry PRIVATE FORCE_BERRY)
target_compile_options(compile_wheat_berry PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(compile_wheat_berry PRIVATE DaySolver::scheduler)
