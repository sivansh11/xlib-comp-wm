cmake_minimum_required(VERSION 3.10)

project(test)

file(GLOB_RECURSE SRC_FILES ./*.cpp)

add_executable(test ${SRC_FILES})

include_directories(test
    ../../window_manager
)

target_link_libraries(test
    window_manager
)
