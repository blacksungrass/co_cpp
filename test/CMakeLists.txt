

add_executable(test1 test1.cpp)
target_include_directories(test1 PRIVATE ../src)
target_sources(test1 PRIVATE ../src/co_env.cpp ../src/co_env.S)
add_test(
    NAME test1
    COMMAND $<TARGET_FILE:test1>
)