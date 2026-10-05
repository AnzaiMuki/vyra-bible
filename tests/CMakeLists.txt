# Unit tests of the code that does not depend on OBS or Qt.
# Standalone project: it needs only a C++17 compiler and CMake (no OBS download).
#
#   cmake -S tests -B build_tests
#   cmake --build build_tests
#   ctest --test-dir build_tests --output-on-failure

cmake_minimum_required(VERSION 3.20)
project(vyra-bible-tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

enable_testing()

get_filename_component(VYRA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

add_executable(bible_module_test bible_module_test.cpp "${VYRA_ROOT}/src/bible/bible_module.cpp")
target_include_directories(bible_module_test PRIVATE "${VYRA_ROOT}")
target_compile_definitions(bible_module_test PRIVATE VYRA_LSG_PATH="${VYRA_ROOT}/data/bibles/lsg1910.tsv")
if(MSVC)
  target_compile_options(bible_module_test PRIVATE /W4 /utf-8)
else()
  target_compile_options(bible_module_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME bible_module COMMAND bible_module_test)
