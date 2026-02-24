cmake_minimum_required(VERSION 3.25)
project(pixelforge VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC: treat all source files as UTF-8 (required for non-ASCII characters in
# comments and string literals, e.g. box-drawing dividers and degree symbols)
if(MSVC)
    add_compile_options(/utf-8)
endif()

# vcpkg toolchain is typically set by the CI / user via -DCMAKE_TOOLCHAIN_FILE
# Fallback: look for it in common locations.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# ── Options ──────────────────────────────────────────────────────────────────
option(PF_BUILD_EDITOR   "Build the PixelForge editor"    ON)
option(PF_BUILD_SANDBOX  "Build the sandbox application"  ON)
option(PF_BUILD_TESTS    "Build the test suite"           ON)

# ── Sub-projects ─────────────────────────────────────────────────────────────
add_subdirectory(engine)

if(PF_BUILD_EDITOR)
    add_subdirectory(editor)
endif()

if(PF_BUILD_SANDBOX)
    add_subdirectory(sandbox)
endif()

if(PF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
