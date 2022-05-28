cmake_minimum_required(VERSION 3.5)

#option(CMAKE_BUILD_TYPE "build type" Release)

project(co_cpp CXX ASM)

include_directories(src/)
include_directories(examples/utils/)

add_subdirectory(examples/basic)
add_subdirectory(examples/echo_server)
add_subdirectory(examples/socks5_server)

enable_testing()
add_subdirectory(test/)