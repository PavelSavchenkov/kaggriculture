cmake_minimum_required(VERSION 3.18)
project(sep24_BC_opus LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)  # static libraries also link into the Local-LB bridge
# Repository root: the nearest parent with agents/common (the original lived 3 levels deep in
# experiments/v10/sep24_BC_opus; this snapshot also builds in place in pipeline_sep25_handoff/).
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
while(NOT EXISTS "${REPO_ROOT}/agents/common/api/agent_api.hpp")
    get_filename_component(PARENT "${REPO_ROOT}" DIRECTORY)
    if(PARENT STREQUAL REPO_ROOT)
        message(FATAL_ERROR "no kaggriculture repository root above ${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
    set(REPO_ROOT "${PARENT}")
endwhile()

# BC_PORTABLE: baseline x86-64 code and a static C++ runtime for the bundled bridge
# (Local-LB / Kaggle agents run on other machines).
option(BC_PORTABLE "portable CPU flags for the agent bundle" OFF)
if(BC_PORTABLE)
    add_compile_options(-O3 -march=x86-64 -mtune=generic -Wall -Wextra -fno-math-errno)
else()
    add_compile_options(-O3 -march=native -mtune=native -Wall -Wextra -fno-math-errno)
endif()

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(day_policy_local)

# Day compiler copied from experiments/v10/sep24_day_compiler_opus (hashes in data/COMPILER_SOURCE.sha256).
add_library(dc10 STATIC source/intent.cpp source/convert.cpp source/compiler.cpp)
target_include_directories(dc10 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${REPO_ROOT}")
target_link_libraries(dc10 PUBLIC day_policy::policy)

add_library(trace_hash SHARED source/trace_hash.cpp)

set(TOOLS "")
file(GLOB TOOL_SOURCES CONFIGURE_DEPENDS tools/*.cpp)
foreach(source ${TOOL_SOURCES})
    get_filename_component(tool ${source} NAME_WE)
    add_executable(${tool} ${source})
    target_link_libraries(${tool} PRIVATE dc10)
endforeach()

set(BUILD_TESTING ON)
enable_testing()
add_executable(mechanics tests/mechanics.cpp)
target_link_libraries(mechanics PRIVATE dc10)
add_test(NAME mechanics COMMAND mechanics)

# Agent under test and the strongest earlier BC agent as an opponent (root resource).
add_library(bc_opus_agent STATIC agent/bc_opus/source/agent.cpp)
target_link_libraries(bc_opus_agent PUBLIC dc10)
target_compile_definitions(bc_opus_agent PRIVATE BC_OPUS_DEFAULT_MODEL="${CMAKE_CURRENT_SOURCE_DIR}/models/selected/model.bin")
add_subdirectory("${REPO_ROOT}/agent_sep23" agent_sep23)
add_library(sep23_adapter STATIC opponents/sep23_adapter.cpp)
target_link_libraries(sep23_adapter PRIVATE agent_sep23)
target_link_libraries(continuation PRIVATE bc_opus_agent)
target_link_libraries(full_games PRIVATE bc_opus_agent sep23_adapter)
target_link_libraries(parity PRIVATE bc_opus_agent)
target_link_libraries(decode_eval PRIVATE bc_opus_agent)
target_link_libraries(replay_games PRIVATE bc_opus_agent)

# Local opponents (agents/inhouse or agents/external, built from their agent.json manifests).
set(OPPONENTS inhouse/two_random_shop_league_v179 inhouse/one_shop_no_geese_league_winner_v1
    external/king_rc4 external/teammate_shoprouter external/investment_context_guarded_001_best
    external/arlene_v4_m31 external/ahmed_v25)
set(DECLARATIONS "")
set(DISPATCH "")
set(EXPERIMENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
foreach(AGENT_PATH ${OPPONENTS})
    get_filename_component(AGENT_NAME "${AGENT_PATH}" NAME)
    set(AGENT_DIR "${REPO_ROOT}/agents/${AGENT_PATH}")
    file(READ "${AGENT_DIR}/agent.json" manifest)
    string(JSON AGENT_HEADER GET "${manifest}" header)
    string(JSON AGENT_TYPE GET "${manifest}" type)
    string(JSON count LENGTH "${manifest}" sources)
    math(EXPR last "${count} - 1")
    set(sources "")
    foreach(i RANGE ${last})
        string(JSON source GET "${manifest}" sources ${i})
        list(APPEND sources "${AGENT_DIR}/${source}")
    endforeach()
    configure_file(opponents/adapter.cpp.in "${CMAKE_CURRENT_BINARY_DIR}/opponent_${AGENT_NAME}.cpp" @ONLY)
    add_library(opp_${AGENT_NAME} STATIC ${sources} "${CMAKE_CURRENT_BINARY_DIR}/opponent_${AGENT_NAME}.cpp")
    target_include_directories(opp_${AGENT_NAME} PRIVATE "${AGENT_DIR}" "${REPO_ROOT}")
    target_compile_options(opp_${AGENT_NAME} PRIVATE -w)
    list(APPEND OPPONENT_LIBS opp_${AGENT_NAME})
    string(APPEND DECLARATIONS "std::unique_ptr<Opponent> make_${AGENT_NAME}();\n")
    string(APPEND DISPATCH "    if (!std::strcmp(name, \"${AGENT_NAME}\")) return make_${AGENT_NAME}();\n")
endforeach()
configure_file(opponents/registry.cpp.in "${CMAKE_CURRENT_BINARY_DIR}/opponent_registry.cpp" @ONLY)
add_library(opponent_registry STATIC "${CMAKE_CURRENT_BINARY_DIR}/opponent_registry.cpp")
target_include_directories(opponent_registry PRIVATE "${REPO_ROOT}")
target_link_libraries(opponent_registry PUBLIC ${OPPONENT_LIBS})
target_link_libraries(full_games PRIVATE opponent_registry)
target_link_libraries(search_games PRIVATE bc_opus_agent opponent_registry)

# Local-LB evaluation bridge (scripts/lb_play.py): the agent behind a C interface.
add_library(opus_lb_bridge SHARED source/lb_bridge.cpp)
target_link_libraries(opus_lb_bridge PRIVATE bc_opus_agent)
if(BC_PORTABLE)
    target_link_options(opus_lb_bridge PRIVATE -static-libstdc++ -static-libgcc)
endif()
target_link_libraries(lb_replay PRIVATE opus_lb_bridge)
