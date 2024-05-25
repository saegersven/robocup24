cmake_minimum_required(VERSION 3.16)
project(display_test VERSION 1)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS -Wall)

find_package(SDL2 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/libs)

add_executable(display_test src/main.c src/display/display.c)

target_link_libraries(display_test SDL2::SDL2)
target_link_libraries(display_test m)
