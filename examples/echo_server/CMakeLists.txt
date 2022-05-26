cmake_minimum_required(VERSION 3.5)
project(echo_server)
add_executable(echo_server 
                test_echo_server.cpp 
                echo_server.cpp 
                ../../src/co_env.cpp 
                ../../src/connection.cpp 
                ../utils/log.cpp)