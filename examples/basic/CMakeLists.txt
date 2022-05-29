add_executable(basic)
target_sources(basic 
                PRIVATE 
                main.cpp
                ../../src/co_env.cpp
                ../../src/co_env.S
                ../utils/log.cpp
                ../../src/co_event_manager.cpp)
