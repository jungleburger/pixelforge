cmake_minimum_required(VERSION 3.25)

find_package(SDL3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)

add_executable(pixelforge_sandbox
    main.cpp
)

target_link_libraries(pixelforge_sandbox PRIVATE
    pixelforge_engine
    SDL3::SDL3
    glad::glad
    sol2::sol2
)

target_compile_features(pixelforge_sandbox PRIVATE cxx_std_23)

# Copy assets next to the binary
add_custom_command(TARGET pixelforge_sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:pixelforge_sandbox>/assets
)
