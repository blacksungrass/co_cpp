cmake_minimum_required(VERSION 3.5)

#option(CMAKE_BUILD_TYPE "build type" Release)
set(CMAKE_CXX_STANDARD 17)

project(co_cpp CXX ASM)

include_directories(src/)
include_directories(examples/utils/)


add_library(co_cpp
            STATIC
            src/co_env.cpp 
            src/co_event_manager.cpp 
            src/connection.cpp
            src/co_thread.cpp
            src/co_thread.S)

add_subdirectory(examples/basic)
add_subdirectory(examples/echo_server)
add_subdirectory(examples/socks5_server)

enable_testing()
add_subdirectory(test/) 