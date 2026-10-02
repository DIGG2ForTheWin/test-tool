cmake_minimum_required(VERSION 4.0)
project(test_tool)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(/opt/homebrew/include)
add_executable(main src/main.cpp)
