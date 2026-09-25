# Analysis tools of the weakness study (work/sep25_bc_weakness), built against the experiment's
# targets through add_subdirectory: hand-linking against its static libraries crashed or changed
# decisions (ODR). forecast_audit exists in both folders; this one builds as forecast_audit_ops.
# usage: cmake -S pipeline_sep25_handoff/analysis -B <build dir> && cmake --build <build dir> -j 8
cmake_minimum_required(VERSION 3.18)
project(sep25_analysis LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(EXPERIMENT "${CMAKE_CURRENT_SOURCE_DIR}/../experiment" CACHE PATH "experiment folder")
add_subdirectory("${EXPERIMENT}" experiment)

file(GLOB TOOL_SOURCES CONFIGURE_DEPENDS tools/*.cpp)
foreach(source ${TOOL_SOURCES})
    get_filename_component(tool ${source} NAME_WE)
    if(TARGET ${tool})
        set(tool ${tool}_ops)
    endif()
    add_executable(${tool} ${source})
    target_link_libraries(${tool} PRIVATE dc10)
endforeach()
target_link_libraries(full_games_reports PRIVATE bc_opus_agent sep23_adapter opponent_registry)
