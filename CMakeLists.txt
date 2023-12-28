cmake_minimum_required(VERSION 3.16)
project(robocup VERSION 24)

set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS -Wall)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(SDL2 REQUIRED)
find_package(Threads REQUIRED)
# find_package(WiringPi REQUIRED)
find_library(WIRINGPI_LIBRARIES NAMES wiringPi)
find_path(WIRINGPI_INCLUDE_DIRS NAMES wiringPi.h)

add_library(tflite SHARED IMPORTED)
set_target_properties(tflite PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libs/libtensorflowlite_c.so)
target_include_directories(tflite
    INTERFACE ${CMAKE_SOURCE_DIR}/libs)

include_directories(${WIRINGPI_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/libs)

add_executable(robocup
    src/main.c
    src/robot.c
    src/utils.c
    src/vision.c
    src/camera.c
    src/line/line.c
    src/rescue/rescue.c
    src/display/display.c)

target_link_libraries(robocup v4l2)
target_link_libraries(robocup Threads::Threads)
target_link_libraries(robocup SDL2::SDL2)
target_link_libraries(robocup m) # math
target_link_libraries(robocup ${WIRINGPI_LIBRARIES})
target_link_libraries(robocup tflite)
target_link_libraries(robocup atomic)