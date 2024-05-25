cmake_minimum_required(VERSION 3.16)
project(tflite_c_api_test VERSION 1)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS -Wall)

include_directories(${CMAKE_SOURCE_DIR}/libs)

add_library(tflite SHARED IMPORTED)
set_target_properties(tflite PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libs/libtensorflowlite_c.so)
target_include_directories(tflite
    INTERFACE ${CMAKE_SOURCE_DIR}/libs)

add_executable(tflite_c_api_test src/main.c)

target_link_libraries(tflite_c_api_test tflite)
