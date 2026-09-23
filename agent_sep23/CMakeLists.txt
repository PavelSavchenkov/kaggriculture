cmake_minimum_required(VERSION 3.18)
project(agent_sep23 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(KAGGRICULTURE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_subdirectory(day_compiler)
add_library(agent_sep23 STATIC source/agent.cpp)
target_link_libraries(agent_sep23 PUBLIC day_compiler::release)
target_include_directories(agent_sep23 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/source" "${KAGGRICULTURE_ROOT}")
target_compile_definitions(agent_sep23 PRIVATE BC_DEFAULT_MODEL_PATH="${CMAKE_CURRENT_SOURCE_DIR}/model/model.bin")
target_compile_options(agent_sep23 PRIVATE -Wall -Wextra -fno-exceptions -fno-rtti $<$<CONFIG:Release>:-O3>)

enable_testing()
add_executable(agent_sep23_full_game tests/full_game.cpp)
target_link_libraries(agent_sep23_full_game PRIVATE agent_sep23)
add_test(NAME production_full_game COMMAND agent_sep23_full_game)
