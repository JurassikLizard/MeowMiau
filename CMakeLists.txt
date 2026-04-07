cmake_minimum_required(VERSION 3.16)
project(meowmiau C)

set(CMAKE_C_STANDARD 11)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

add_subdirectory(external/raylib)

add_executable(meowmiau
    src/main.c
)

target_include_directories(meowmiau PRIVATE
    external/raylib/src
)

target_link_libraries(meowmiau raylib)