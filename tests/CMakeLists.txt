cmake_minimum_required(VERSION 3.25)

find_package(Catch2 3 CONFIG REQUIRED)

add_executable(pixelforge_tests
    test_main.cpp
    test_element_registry.cpp
    test_lattice.cpp
    test_bonding.cpp
    test_physics.cpp
    test_procgen.cpp
)

target_link_libraries(pixelforge_tests PRIVATE
    pixelforge_engine
    Catch2::Catch2WithMain
)

target_compile_features(pixelforge_tests PRIVATE cxx_std_23)

include(CTest)
include(Catch)
catch_discover_tests(pixelforge_tests)
