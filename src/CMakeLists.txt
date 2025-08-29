add_library(log_parser)

add_subdirectory(line_parsing)
add_subdirectory(file_parsing)
add_subdirectory(frontends)

target_link_libraries(log_parser
    PUBLIC
        line_parsing
        file_parsing
)

add_executable(lp main.cpp)
target_link_libraries(lp PUBLIC log_parser)