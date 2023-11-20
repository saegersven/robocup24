cmake_minimum_required(VERSION 3.16)
project(spi_test VERSION 24)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS -Wall)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

add_executable(spi_test src/main.c)

target_link_libraries(spi_test Threads::Threads)
target_link_libraries(spi_test m) # math
