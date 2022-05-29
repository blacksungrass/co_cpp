add_executable(socks5_server)
target_sources(socks5_server 
                PRIVATE
                test_socks5_server.cpp 
                socks5_server.cpp 
                socks5_client.cpp
                ../utils/log.cpp)
target_link_libraries(socks5_server co_cpp)