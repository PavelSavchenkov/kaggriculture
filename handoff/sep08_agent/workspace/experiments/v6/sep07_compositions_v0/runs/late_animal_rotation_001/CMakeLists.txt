cmake_minimum_required(VERSION 3.18)
project(late_animal_rotation LANGUAGES CXX)
get_filename_component(EXPERIMENT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
get_filename_component(REPOSITORY "${EXPERIMENT}/../../.." ABSOLUTE)
add_subdirectory("${REPOSITORY}/day_solver" day_solver)
foreach(leaf off on)
    add_executable(estimate_${leaf} source/estimate.cpp
        "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
        "${EXPERIMENT}/league/public_router/source/agent.cpp")
    target_include_directories(estimate_${leaf} BEFORE PRIVATE "${REPOSITORY}")
    target_compile_options(estimate_${leaf} PRIVATE -O3 -march=native -mtune=native)
    target_link_libraries(estimate_${leaf} PRIVATE DaySolver::scheduler)
endforeach()
target_compile_definitions(estimate_on PRIVATE LATE_BERRY=1)

foreach(leaf off on)
    add_executable(compile_${leaf} source/compile.cpp
        "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
        "${EXPERIMENT}/league/public_router/source/agent.cpp")
    target_include_directories(compile_${leaf} BEFORE PRIVATE "${REPOSITORY}")
    target_compile_options(compile_${leaf} PRIVATE -O3 -march=native -mtune=native)
    target_link_libraries(compile_${leaf} PRIVATE DaySolver::scheduler)
endforeach()
target_compile_definitions(compile_on PRIVATE LATE_BERRY=1)
