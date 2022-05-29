

add_executable(test1 test1.cpp)
target_include_directories(test1 PRIVATE ../src)
target_sources(test1 PRIVATE ../src/co_env.cpp ../src/co_env.S ../src/co_event_manager.cpp)
add_test(
    NAME test1
    COMMAND $<TARGET_FILE:test1>
)

add_executable(test2 test2.cpp)
target_include_directories(test2 PRIVATE ../src)
target_sources(test2
                PRIVATE 
                ../src/co_env.cpp 
                ../src/co_env.S 
                ../src/co_event_manager.cpp
                ../src/connection.cpp)
add_test(
    NAME test2
    COMMAND $<TARGET_FILE:test2>
)