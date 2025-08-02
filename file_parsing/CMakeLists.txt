
# Cmake
include_directories(../line_parsing)

add_library(filtering SHARED line_filter.cpp)

add_library(log_parser SHARED file_parser.cpp)
target_link_libraries(log_parser line_parsing filtering)