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
  "${VYRA_ROOT}/src/stage/paginator.cpp"
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
find_package(Qt6 QUIET COMPONENTS Widgets Test Network)
if(Qt6_FOUND)
  set(CMAKE_AUTOMOC ON)
  add_executable(
    overlay_server_test
    overlay_server_test.cpp
    "${VYRA_ROOT}/src/overlay/overlay_server.cpp"
    "${VYRA_ROOT}/src/overlay/overlay_frame.cpp"
    "${VYRA_ROOT}/src/overlay/http_routes.cpp"
    "${VYRA_ROOT}/src/stage/stage_controller.cpp"
    "${VYRA_ROOT}/src/stage/paginator.cpp"
    "${VYRA_ROOT}/src/bible/bible_module.cpp"
    "${VYRA_ROOT}/src/search/book_catalog.cpp"
    "${VYRA_ROOT}/src/search/passage_query.cpp"
  )
  target_include_directories(overlay_server_test PRIVATE "${VYRA_ROOT}")
  target_compile_definitions(overlay_server_test PRIVATE VYRA_OVERLAY_DIR="${VYRA_ROOT}/data/overlay")
  target_link_libraries(overlay_server_test PRIVATE Qt6::Core Qt6::Network)
  add_test(NAME overlay_server COMMAND overlay_server_test)

  # Helper for tests/overlay_e2e_test.py (a browser against the real server); not a test by itself.
  add_executable(
    overlay_demo
    overlay_demo.cpp
    "${VYRA_ROOT}/src/overlay/overlay_server.cpp"
    "${VYRA_ROOT}/src/overlay/overlay_frame.cpp"
    "${VYRA_ROOT}/src/overlay/http_routes.cpp"
    "${VYRA_ROOT}/src/stage/stage_controller.cpp"
    "${VYRA_ROOT}/src/stage/paginator.cpp"
    "${VYRA_ROOT}/src/bible/bible_module.cpp"
    "${VYRA_ROOT}/src/search/book_catalog.cpp"
    "${VYRA_ROOT}/src/search/passage_query.cpp"
  )
  target_include_directories(overlay_demo PRIVATE "${VYRA_ROOT}")
  target_compile_definitions(
    overlay_demo
    PRIVATE
      VYRA_LSG_PATH="${VYRA_ROOT}/data/bibles/lsg1910.tsv"
      VYRA_OVERLAY_DIR="${VYRA_ROOT}/data/overlay"
  )
  target_link_libraries(overlay_demo PRIVATE Qt6::Core Qt6::Network)

  add_executable(
    dock_ui_test
    dock_ui_test.cpp
    "${VYRA_ROOT}/ui/vyra_dock.cpp"
    "${VYRA_ROOT}/ui/stage_panel.cpp"
    "${VYRA_ROOT}/ui/picker_panel.cpp"
    "${VYRA_ROOT}/src/bible/bible_module.cpp"
    "${VYRA_ROOT}/src/search/book_catalog.cpp"
    "${VYRA_ROOT}/src/search/passage_query.cpp"
    "${VYRA_ROOT}/src/stage/stage_controller.cpp"
    "${VYRA_ROOT}/src/stage/paginator.cpp"
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

add_executable(
  paginator_test
  paginator_test.cpp
  "${VYRA_ROOT}/src/bible/bible_module.cpp"
  "${VYRA_ROOT}/src/search/book_catalog.cpp"
  "${VYRA_ROOT}/src/search/passage_query.cpp"
  "${VYRA_ROOT}/src/stage/stage_controller.cpp"
  "${VYRA_ROOT}/src/stage/paginator.cpp"
)
target_include_directories(paginator_test PRIVATE "${VYRA_ROOT}")
target_compile_definitions(paginator_test PRIVATE VYRA_LSG_PATH="${VYRA_ROOT}/data/bibles/lsg1910.tsv")
if(MSVC)
  target_compile_options(paginator_test PRIVATE /W4 /utf-8)
else()
  target_compile_options(paginator_test PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME paginator COMMAND paginator_test)

add_executable(
  frames_dump
  frames_dump.cpp
  "${VYRA_ROOT}/src/bible/bible_module.cpp"
  "${VYRA_ROOT}/src/search/book_catalog.cpp"
  "${VYRA_ROOT}/src/search/passage_query.cpp"
  "${VYRA_ROOT}/src/stage/stage_controller.cpp"
  "${VYRA_ROOT}/src/stage/paginator.cpp"
  "${VYRA_ROOT}/src/overlay/overlay_frame.cpp"
)
target_include_directories(frames_dump PRIVATE "${VYRA_ROOT}")
target_compile_definitions(frames_dump PRIVATE VYRA_LSG_PATH="${VYRA_ROOT}/data/bibles/lsg1910.tsv")

add_executable(
  overlay_test
  overlay_test.cpp
  "${VYRA_ROOT}/src/bible/bible_module.cpp"
  "${VYRA_ROOT}/src/search/book_catalog.cpp"
  "${VYRA_ROOT}/src/search/passage_query.cpp"
  "${VYRA_ROOT}/src/stage/stage_controller.cpp"
  "${VYRA_ROOT}/src/stage/paginator.cpp"
  "${VYRA_ROOT}/src/overlay/overlay_frame.cpp"
  "${VYRA_ROOT}/src/overlay/http_routes.cpp"
)
target_include_directories(overlay_test PRIVATE "${VYRA_ROOT}")
target_compile_definitions(overlay_test PRIVATE VYRA_LSG_PATH="${VYRA_ROOT}/data/bibles/lsg1910.tsv")
if(MSVC)
  target_compile_options(overlay_test PRIVATE /W4 /utf-8)
else()
  target_compile_options(overlay_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME overlay COMMAND overlay_test)

# Browser tests of the overlay page: only when Python can import Playwright (and a Chromium is installed).
find_package(Python3 QUIET COMPONENTS Interpreter)
if(Python3_FOUND)
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c "import playwright"
    RESULT_VARIABLE VYRA_PLAYWRIGHT_MISSING
    OUTPUT_QUIET
    ERROR_QUIET
  )
  if(NOT VYRA_PLAYWRIGHT_MISSING)
    add_test(
      NAME overlay_page
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/overlay_page_test.py" "" "$<TARGET_FILE:frames_dump>"
    )
    if(TARGET overlay_demo AND NOT WIN32)
      add_test(
        NAME overlay_e2e
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/overlay_e2e_test.py" "$<TARGET_FILE:overlay_demo>"
      )
    endif()
  endif()
endif()
