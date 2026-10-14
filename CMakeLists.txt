cmake_minimum_required(VERSION 3.20)
project(test_tool CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to a Debug build if no build type was given on the command line
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# External dependencies
find_package(yaml-cpp REQUIRED)

# Flags for every target defined below
add_compile_options(-Wall -Wextra -Wshadow)

# AddressSanitizer only in Debug builds (never in Release / on AWS)
add_compile_options($<$<CONFIG:Debug>:-fsanitize=address>)
add_compile_options($<$<CONFIG:Debug>:-fno-omit-frame-pointer>)
add_link_options($<$<CONFIG:Debug>:-fsanitize=address>)

# Shared code, used by both programs
add_library(tester_core STATIC
    src/cli.cpp
    src/node_config.cpp
    src/run_config.cpp
    src/yaml_util.cpp
    src/test_config.cpp
    src/size.cpp
    src/generate.cpp
    src/metadata.cpp
    src/io_util.cpp
)
target_include_directories(tester_core PUBLIC src)
target_link_libraries(tester_core PRIVATE yaml-cpp::yaml-cpp)

# The real program
add_executable(tester src/main.cpp)
target_link_libraries(tester PRIVATE tester_core)

# Small tool for testing file generation on its own
add_executable(gen_bench tools/gen_bench.cpp)
target_link_libraries(gen_bench PRIVATE tester_core)