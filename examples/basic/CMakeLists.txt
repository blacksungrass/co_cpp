cmake_minimum_required(VERSION 3.5)
#project(basic CXX ASM)
add_executable(basic main.cpp)

target_sources(basic PRIVATE ../../src/co_env.cpp
                             ../../src/co_env.S
                             ../utils/log.cpp)