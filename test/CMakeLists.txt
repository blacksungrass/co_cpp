

add_executable(test1 test1.cpp)
target_link_libraries(test1 co_cpp)
add_test(
    NAME test1
    COMMAND $<TARGET_FILE:test1>
)

add_executable(test2 test2.cpp)
target_link_libraries(test2 co_cpp)
add_test(
    NAME test2
    COMMAND $<TARGET_FILE:test2>
)