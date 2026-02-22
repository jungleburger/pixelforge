cmake_minimum_required(VERSION 3.25)

find_package(SDL3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Collect all editor sources
set(EDITOR_SOURCES
    src/main.cpp
    src/editor_app.cpp
    src/panels/viewport_panel.cpp
    src/panels/hierarchy_panel.cpp
    src/panels/inspector_panel.cpp
    src/panels/element_palette_panel.cpp
    src/panels/element_editor_panel.cpp
    src/panels/worldgen_panel.cpp
    src/panels/asset_browser_panel.cpp
    src/panels/console_panel.cpp
    src/panels/performance_panel.cpp
    src/tools/paint_tool.cpp
    src/tools/erase_tool.cpp
    src/tools/select_tool.cpp
)

add_executable(pixelforge_editor ${EDITOR_SOURCES})

target_include_directories(pixelforge_editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(pixelforge_editor PRIVATE
    pixelforge_engine
    SDL3::SDL3
    glad::glad
    imgui::imgui
)

target_compile_features(pixelforge_editor PRIVATE cxx_std_23)
