add_executable(basic)
target_sources(basic 
                PRIVATE 
                main.cpp
                ../utils/log.cpp)
target_link_libraries(basic co_cpp)
