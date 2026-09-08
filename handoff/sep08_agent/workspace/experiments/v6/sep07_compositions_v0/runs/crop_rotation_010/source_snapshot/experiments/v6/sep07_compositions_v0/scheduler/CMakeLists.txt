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
add_executable(compile_animal_entries
    "${EXPERIMENT}/src/compile_animal_entries.cpp"
    "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
    "${EXPERIMENT}/league/king_rc4/source/agent.cpp"
    "${EXPERIMENT}/league/public_router/source/agent.cpp")
target_include_directories(compile_animal_entries BEFORE PRIVATE "${REPOSITORY}")
target_compile_options(compile_animal_entries PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(compile_animal_entries PRIVATE DaySolver::scheduler)
add_executable(rebuild_investment_days
    "${EXPERIMENT}/src/rebuild_investment_days.cpp"
    "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
    "${EXPERIMENT}/league/public_router/source/agent.cpp")
target_include_directories(rebuild_investment_days BEFORE PRIVATE "${REPOSITORY}")
target_compile_options(rebuild_investment_days PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(rebuild_investment_days PRIVATE DaySolver::scheduler)
add_executable(compile_general_animal_entries
    "${EXPERIMENT}/src/compile_animal_entries.cpp"
    "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
    "${EXPERIMENT}/league/king_rc4/source/agent.cpp"
    "${EXPERIMENT}/league/public_router/source/agent.cpp")
target_include_directories(compile_general_animal_entries BEFORE PRIVATE "${REPOSITORY}")
target_compile_definitions(compile_general_animal_entries PRIVATE GENERAL_ANIMAL_ENTRIES)
target_compile_options(compile_general_animal_entries PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(compile_general_animal_entries PRIVATE DaySolver::scheduler)
add_executable(improve_fertilization
    "${EXPERIMENT}/src/improve_fertilization.cpp"
    "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
    "${EXPERIMENT}/league/public_router/source/agent.cpp")
target_include_directories(improve_fertilization BEFORE PRIVATE "${REPOSITORY}")
target_compile_options(improve_fertilization PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(improve_fertilization PRIVATE DaySolver::scheduler)
add_executable(compile_crop_rotation
    "${EXPERIMENT}/src/compile_crop_rotation.cpp"
    "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
    "${EXPERIMENT}/league/public_router/source/agent.cpp")
target_include_directories(compile_crop_rotation BEFORE PRIVATE "${REPOSITORY}")
target_compile_options(compile_crop_rotation PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(compile_crop_rotation PRIVATE DaySolver::scheduler)
add_executable(compile_crop_rotation_berry
    "${EXPERIMENT}/src/compile_crop_rotation.cpp"
    "${EXPERIMENT}/league/top_replay_library/source/agent.cpp"
    "${EXPERIMENT}/league/public_router/source/agent.cpp")
target_include_directories(compile_crop_rotation_berry BEFORE PRIVATE "${REPOSITORY}")
target_compile_definitions(compile_crop_rotation_berry PRIVATE COMPOSITION_ROTATION_BERRY_SOURCE)
target_compile_options(compile_crop_rotation_berry PRIVATE -O3 -march=native -mtune=native)
target_link_libraries(compile_crop_rotation_berry PRIVATE DaySolver::scheduler)
