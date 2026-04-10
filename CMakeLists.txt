cmake_minimum_required(VERSION 3.16)
project(meowmiau C)

set(CMAKE_C_STANDARD 11)

# Configure external dependencies
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

# Add external projects
add_subdirectory(external/raylib)

# Use CONFIGURE_DEPENDS so adding a file triggers a re-run
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.c")

# Main executable
add_executable(meowmiau
    ${SOURCES}
)

# Include directories
target_include_directories(meowmiau PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    external/raylib/src
    external/include
)

# Link libraries
target_link_libraries(meowmiau PRIVATE
    raylib
)

# Copy assets to build directory
add_custom_command(TARGET meowmiau POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:meowmiau>/assets"
)