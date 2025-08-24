# ================= CONFIGURING ===================================
cmake_minimum_required(VERSION 3.21)

project(LogParser LANGUAGES CXX VERSION 0.0)

set(CMAKE_BUILD_DIR ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_BINARY_DIR ${CMAKE_BUILD_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BUILD_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BUILD_DIR}/lib)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ================= BUILDING ======================================

include_directories(line_parsing)
add_subdirectory(line_parsing)
include_directories(file_parsing)
add_subdirectory(file_parsing)

add_executable(LogParser main.cpp)

add_subdirectory(frontends)

# ================= LINKING ======================================

target_link_libraries(LogParser line_parsing filtering)
