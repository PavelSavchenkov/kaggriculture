cmake_minimum_required(VERSION 3.18)
project(opus_lb_bridge LANGUAGES CXX)

# Builds only libopus_lb_bridge.so (dc11 bridge) from ../source, with the repository headers it needs
# copied into include/. Same flags as the shipped build (source/CMakeLists.txt with BC_PORTABLE=ON
# and BRIDGE_DC11=ON).
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_compile_options(-O3 -march=x86-64 -mtune=generic -Wall -Wextra -fno-math-errno)

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/../source")
set(INC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(POLICY "${SRC}/day_policy_local/source")

add_library(contract_policy STATIC
    ${POLICY}/agent.cpp ${POLICY}/routes.cpp ${POLICY}/policy.cpp ${POLICY}/verify.cpp)
target_include_directories(contract_policy PUBLIC "${INC}" "${POLICY}")
target_compile_options(contract_policy PRIVATE -fno-exceptions -fno-rtti)

add_library(opus_lb_bridge SHARED
    ${SRC}/source/lb_bridge.cpp ${SRC}/agent/bc_opus/source/agent.cpp ${SRC}/agent/bc_overhaul/source/agent.cpp
    ${SRC}/source/intent.cpp ${SRC}/source/convert.cpp ${SRC}/source/compiler.cpp
    ${SRC}/dc11/compiler.cpp ${SRC}/dc11/market.cpp ${SRC}/dc11/router.cpp)
target_include_directories(opus_lb_bridge PRIVATE "${SRC}" "${INC}")
# main.py sets BC_OPUS_MODEL before each game; these defaults are never used.
target_compile_definitions(opus_lb_bridge PRIVATE BRIDGE_DC11 BC_OPUS_DEFAULT_MODEL="model/model.bin"
                           BC_OVERHAUL_DEFAULT_MODEL="model/model.bin")
target_link_libraries(opus_lb_bridge PRIVATE contract_policy)
target_link_options(opus_lb_bridge PRIVATE -static-libstdc++ -static-libgcc)
