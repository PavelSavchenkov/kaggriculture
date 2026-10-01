cmake_minimum_required(VERSION 3.18)
project(sep24_BC_opus LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)  # static libraries also link into the Local-LB bridge
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)  # trees/<name>: five levels below the repo root (copy of work/sep29_fund/src_m14)

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

# dc11 day compiler (vendored from experiments/v10/sep25_compiler_overhaul, see dc11/VENDORED.md): our seat in
# full_games_dc11 (tools/full_games.cpp built with MINE_DC11).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dc11/compiler.cpp")
    file(GLOB DC11_SOURCES CONFIGURE_DEPENDS dc11/*.cpp)
    add_library(dc11 STATIC ${DC11_SOURCES})
    target_link_libraries(dc11 PUBLIC dc10)
    add_library(bc_overhaul STATIC agent/bc_overhaul/source/agent.cpp)
    target_link_libraries(bc_overhaul PUBLIC dc11 bc_opus_agent)
    target_compile_definitions(bc_overhaul PRIVATE BC_OVERHAUL_DEFAULT_MODEL="${CMAKE_CURRENT_SOURCE_DIR}/models/selected/model.bin")
    add_executable(full_games_dc11 tools/full_games.cpp)
    target_compile_definitions(full_games_dc11 PRIVATE MINE_DC11 WITH_DC11)
    target_link_libraries(full_games_dc11 PRIVATE bc_overhaul sep23_adapter opponent_registry)
    add_executable(replay_games_dc11 tools/replay_games.cpp)
    target_compile_definitions(replay_games_dc11 PRIVATE MINE_DC11)
    target_link_libraries(replay_games_dc11 PRIVATE bc_overhaul)
    add_executable(forecast_extract tools_dc11/forecast_extract.cpp)  # opponent-sales forecast dataset
    target_link_libraries(forecast_extract PRIVATE bc_overhaul)
    add_executable(ei_dc11 tools_dc11/ei_dc11.cpp)  # counterfactual one-dawn decode pushes (expert-iteration probe)
    target_link_libraries(ei_dc11 PRIVATE bc_overhaul)
    add_executable(day_harvests tools_dc11/day_harvests.cpp)  # local (sep29_fund): one seat's dawn crops and wheat use
    target_link_libraries(day_harvests PRIVATE bc_overhaul)
    add_executable(teacher_day tools_dc11/teacher_day.cpp)  # local (sep28_top_lb_imitation): teacher-intent one-day replays
    target_link_libraries(teacher_day PRIVATE bc_overhaul)
    add_executable(duel_dc11 tools_dc11/duel_dc11.cpp)  # copied from sep28_top_lb_imitation: two live agents in a recorded game's world
    target_link_libraries(duel_dc11 PRIVATE bc_overhaul)
    add_executable(options_diff tools_dc11/options_diff.cpp)  # local (Imitation): gate 2, the net's intent vs the label on recorded dawns
    target_link_libraries(options_diff PRIVATE bc_overhaul)
    add_executable(seller_diff tools_dc11/seller_diff.cpp)  # local (Imitation): our seller vs the recorded player per hour on its states (I2 / I4)
    target_link_libraries(seller_diff PRIVATE bc_overhaul)
    add_executable(duel_mm tools_dc11/duel_mm.cpp)  # local (Imitation): duel_dc11 with recorded arms (DUEL_REC / LOAN / REC_UNTIL) on this tree
    target_link_libraries(duel_mm PRIVATE bc_overhaul)
    add_executable(flip_dc11 tools_dc11/flip_dc11.cpp)  # reactive loss-flip search: one-day pushes forked from lost clone-bed games
    target_link_libraries(flip_dc11 PRIVATE bc_overhaul)
    add_executable(full_games_vsdc11 tools/full_games.cpp)  # our seat bc_opus (dc10), dc11: opponents available
    target_compile_definitions(full_games_vsdc11 PRIVATE WITH_DC11)
    target_link_libraries(full_games_vsdc11 PRIVATE bc_overhaul sep23_adapter opponent_registry)
endif()
# Local-LB evaluation bridge (scripts/lb_play.py): the agent behind a C interface.
add_library(opus_lb_bridge SHARED source/lb_bridge.cpp)
target_link_libraries(opus_lb_bridge PRIVATE bc_opus_agent)
# BRIDGE_DC11: the bridge runs the vendored dc11 agent (bc_overhaul) instead of bc_opus (same C interface).
option(BRIDGE_DC11 "Local-LB bridge with the dc11 day compiler" OFF)
if(BRIDGE_DC11)
    target_compile_definitions(opus_lb_bridge PRIVATE BRIDGE_DC11)
    target_link_libraries(opus_lb_bridge PRIVATE bc_overhaul)
endif()
if(BC_PORTABLE)
    target_link_options(opus_lb_bridge PRIVATE -static-libstdc++ -static-libgcc)
endif()
target_link_libraries(lb_replay PRIVATE opus_lb_bridge)
