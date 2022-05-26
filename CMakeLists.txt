cmake_minimum_required(VERSION 3.5)
project(co_cpp)

include_directories(src/)
include_directories(examples/utils/)

add_subdirectory(examples/basic)
add_subdirectory(examples/echo_server)