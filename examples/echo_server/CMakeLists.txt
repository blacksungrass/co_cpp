add_executable(echo_server)

target_sources(echo_server 
                PRIVATE
                test_echo_server.cpp 
                echo_server.cpp 
                ../utils/log.cpp)
target_link_libraries(echo_server co_cpp)