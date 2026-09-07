cmake_minimum_required(VERSION 3.18)
project(composition_day_contracts LANGUAGES CXX)
get_filename_component(EXPERIMENT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
get_filename_component(REPOSITORY "${EXPERIMENT}/../../.." ABSOLUTE)
add_subdirectory("${REPOSITORY}/day_solver" day_solver)
add_executable(rebuild_days
    "${EXPERIMENT}/src/rebuild_days.cpp"
    "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
    "${EXPERIMENT}/league/public_router/source/agent.cpp")
target_include_directories(rebuild_days BEFORE PRIVATE "${REPOSITORY}")
target_compile_options(rebuild_days PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(rebuild_days PRIVATE DaySolver::scheduler)
add_executable(advance_sales
    "${EXPERIMENT}/src/advance_sales.cpp"
    "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
    "${EXPERIMENT}/league/public_router/source/agent.cpp")
target_include_directories(advance_sales BEFORE PRIVATE "${REPOSITORY}")
target_compile_options(advance_sales PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(advance_sales PRIVATE DaySolver::scheduler)
add_executable(improve_care
    "${EXPERIMENT}/src/improve_care.cpp"
    "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
    "${EXPERIMENT}/league/public_router/source/agent.cpp")
target_include_directories(improve_care BEFORE PRIVATE "${REPOSITORY}")
target_compile_options(improve_care PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(improve_care PRIVATE DaySolver::scheduler)
add_executable(rebuild_shop_days
    "${EXPERIMENT}/src/rebuild_shop_days.cpp"
    "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
    "${EXPERIMENT}/league/public_router/source/agent.cpp")
target_include_directories(rebuild_shop_days BEFORE PRIVATE "${REPOSITORY}")
target_compile_options(rebuild_shop_days PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(rebuild_shop_days PRIVATE DaySolver::scheduler)
