cmake_minimum_required(VERSION 3.5)
project(socks5_server)
add_executable(socks5_server 
                test_socks5_server.cpp 
                socks5_server.cpp 
                socks5_client.cpp
                ../../src/co_env.cpp 
                ../../src/connection.cpp 
                ../utils/log.cpp)