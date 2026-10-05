cmake_minimum_required(VERSION 3.20)
project(test_tool)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


add_executable(tester 
    src/main.cpp
    src/cli.cpp
    src/node_config.cpp
)


find_package(yaml-cpp REQUIRED)
target_link_libraries(tester PRIVATE yaml-cpp::yaml-cpp)

target_compile_options(tester PRIVATE -Wall -Wextra -fsanitize=address -g)
target_link_options(tester PRIVATE -fsanitize=address)