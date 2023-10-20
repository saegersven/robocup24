cmake_minimum_required(VERSION 3.16)
project(robocup VERSION 24)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS -Wall)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(SDL2 REQUIRED)
find_package(Threads REQUIRED)
# find_package(WiringPi REQUIRED)
find_library(WIRINGPI_LIBRARIES NAMES wiringPi)
find_path(WIRINGPI_INCLUDE_DIRS NAMES wiringPi.h)

include_directories(${WIRINGPI_INCLUDE_DIRS})

add_executable(robocup src/main.c src/robot.c src/utils.c src/vision.c src/camera.c)

target_link_libraries(robocup v4l2)
target_link_libraries(robocup Threads::Threads)
target_link_libraries(robocup SDL2::SDL2)
target_link_libraries(robocup m) # math
target_link_libraries(robocup ${WIRINGPI_LIBRARIES})