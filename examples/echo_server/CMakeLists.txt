add_executable(echo_server)

target_sources(echo_server 
                PRIVATE
                test_echo_server.cpp 
                echo_server.cpp 
                ../../src/co_env.cpp 
                ../../src/co_env.S
                ../../src/connection.cpp 
                ../../src/co_event_manager.cpp
                ../utils/log.cpp)