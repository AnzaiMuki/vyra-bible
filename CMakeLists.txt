cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 REQUIRED COMPONENTS Widgets Core Network)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/bible/bible_module.cpp
    src/bible/bible_module.hpp
    src/library/passage_library.cpp
    src/library/passage_library.hpp
    src/obs/hotkeys.cpp
    src/obs/hotkeys.hpp
    src/obs/obs_source_setup.cpp
    src/obs/obs_source_setup.hpp
    src/overlay/http_routes.cpp
    src/overlay/http_routes.hpp
    src/overlay/overlay_frame.cpp
    src/overlay/overlay_frame.hpp
    src/overlay/overlay_server.cpp
    src/overlay/overlay_server.hpp
    src/plugin/plugin_main.cpp
    src/search/book_catalog.cpp
    src/search/book_catalog.hpp
    src/search/passage_query.cpp
    src/search/passage_query.hpp
    src/stage/stage_controller.cpp
    src/stage/paginator.cpp
    src/stage/paginator.hpp
    src/stage/operator_action.hpp
    src/stage/slide.hpp
    src/stage/stage_controller.hpp
    ui/picker_panel.cpp
    ui/picker_panel.hpp
    ui/stage_panel.cpp
    ui/stage_panel.hpp
    ui/vyra_dock.cpp
    ui/vyra_dock.hpp
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
