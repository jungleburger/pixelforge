cmake_minimum_required(VERSION 3.25)

find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)

# ── pixelforge_engine static library ─────────────────────────────────────────
add_library(pixelforge_engine STATIC
    src/core/logger.cpp
    src/element/element_def.cpp
    src/element/element_registry.cpp
    src/pixel/settled_pixel.cpp
    src/pixel/dynamic_pixel.cpp
    src/bond/bond.cpp
    src/lattice/lattice.cpp
    src/world/chunk.cpp
    src/world/world.cpp
    src/physics/particle_system.cpp
    src/procgen/noise_utils.cpp
    src/procgen/biome.cpp
    src/procgen/generator.cpp
    src/renderer/gl_renderer.cpp
    src/renderer/settled_layer.cpp
    src/renderer/dynamic_layer.cpp
    src/scripting/lua_api.cpp
    src/save/world_serialiser.cpp
)

target_include_directories(pixelforge_engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(pixelforge_engine
    PUBLIC
        glm::glm
    PRIVATE
        glad::glad
        SDL3::SDL3
        sol2::sol2
        $<IF:$<TARGET_EXISTS:zstd::libzstd_shared>,zstd::libzstd_shared,zstd::libzstd_static>
)

target_compile_features(pixelforge_engine PUBLIC cxx_std_23)
