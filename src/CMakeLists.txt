add_library(log_parser)

target_compile_options(log_parser PRIVATE "${CMAKE_CXX_FLAGS_WARNINGS}")

add_subdirectory(common)
add_subdirectory(line_parsing)
add_subdirectory(file_parsing)

target_link_libraries(log_parser
    PUBLIC
        line_parsing
        file_parsing
)


add_subdirectory(frontends)