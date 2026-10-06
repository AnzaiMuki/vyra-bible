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

add_executable(
  search_test
  search_test.cpp
  "${VYRA_ROOT}/src/bible/bible_module.cpp"
  "${VYRA_ROOT}/src/search/book_catalog.cpp"
  "${VYRA_ROOT}/src/search/passage_query.cpp"
)
target_include_directories(search_test PRIVATE "${VYRA_ROOT}")
target_compile_definitions(
  search_test
  PRIVATE
    VYRA_LSG_PATH="${VYRA_ROOT}/data/bibles/lsg1910.tsv"
    VYRA_CROSSREFS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/data/segond_crossrefs.tsv"
    VYRA_ANOMALIES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/data/segond_crossref_anomalies.tsv"
)
if(MSVC)
  target_compile_options(search_test PRIVATE /W4 /utf-8)
else()
  target_compile_options(search_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME search COMMAND search_test)

add_executable(
  stage_test
  stage_test.cpp
  "${VYRA_ROOT}/src/bible/bible_module.cpp"
  "${VYRA_ROOT}/src/search/book_catalog.cpp"
  "${VYRA_ROOT}/src/search/passage_query.cpp"
  "${VYRA_ROOT}/src/stage/stage_controller.cpp"
)
target_include_directories(stage_test PRIVATE "${VYRA_ROOT}")
target_compile_definitions(stage_test PRIVATE VYRA_LSG_PATH="${VYRA_ROOT}/data/bibles/lsg1910.tsv")
if(MSVC)
  target_compile_options(stage_test PRIVATE /W4 /utf-8)
else()
  target_compile_options(stage_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME stage COMMAND stage_test)

# The dock itself, driven with simulated keys. Optional: needs Qt6 Widgets and Test, but not OBS.
find_package(Qt6 QUIET COMPONENTS Widgets Test)
if(Qt6_FOUND)
  set(CMAKE_AUTOMOC ON)
  add_executable(
    dock_ui_test
    dock_ui_test.cpp
    "${VYRA_ROOT}/ui/vyra_dock.cpp"
    "${VYRA_ROOT}/ui/stage_panel.cpp"
    "${VYRA_ROOT}/src/bible/bible_module.cpp"
    "${VYRA_ROOT}/src/search/book_catalog.cpp"
    "${VYRA_ROOT}/src/search/passage_query.cpp"
    "${VYRA_ROOT}/src/stage/stage_controller.cpp"
  )
  # ui/vyra_dock.cpp includes <obs-module.h> and <plugin-support.h>: only the stubs of tests/stubs are used.
  target_include_directories(dock_ui_test PRIVATE "${VYRA_ROOT}" "${CMAKE_CURRENT_SOURCE_DIR}/stubs")
  target_compile_definitions(
    dock_ui_test
    PRIVATE
      VYRA_LSG_PATH="${VYRA_ROOT}/data/bibles/lsg1910.tsv"
      VYRA_LOCALE_FR="${VYRA_ROOT}/data/locale/fr-FR.ini"
  )
  target_link_libraries(dock_ui_test PRIVATE Qt6::Widgets Qt6::Test)
  add_test(NAME dock_ui COMMAND dock_ui_test)
  set_tests_properties(dock_ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
