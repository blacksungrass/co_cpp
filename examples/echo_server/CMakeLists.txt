cmake_minimum_required(VERSION 3.5)
#project(echo_server CXX ASM)
add_executable(echo_server 
                test_echo_server.cpp 
                echo_server.cpp 
                ../../src/co_env.cpp 
                ../../src/co_env.S
                ../../src/connection.cpp 
                ../utils/log.cpp)