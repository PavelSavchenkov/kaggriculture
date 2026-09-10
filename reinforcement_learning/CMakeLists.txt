cmake_minimum_required(VERSION 3.18)
project(kaggriculture_rl_core LANGUAGES CXX)

find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
execute_process(
  COMMAND "${Python_EXECUTABLE}" -m pybind11 --cmakedir
  OUTPUT_VARIABLE PYBIND11_CMAKE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY)
list(APPEND CMAKE_PREFIX_PATH "${PYBIND11_CMAKE_DIR}")
find_package(pybind11 CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

pybind11_add_module(_core MODULE cpp/core.cpp)
target_compile_features(_core PRIVATE cxx_std_20)
target_compile_options(_core PRIVATE -O3 -march=native)
target_include_directories(_core PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
  "${CMAKE_CURRENT_SOURCE_DIR}/../corpus/include")
target_link_libraries(_core PRIVATE ZLIB::ZLIB)

set_target_properties(_core PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/python/kaggriculture_rl")
