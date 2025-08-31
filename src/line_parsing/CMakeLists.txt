add_library(line_parsing)

target_sources(line_parsing
    PRIVATE 
        parsing_basics.cpp 
        parsing_data.cpp 
        line_parser.cpp
    PUBLIC 
        parsing_basics.hpp
        parsing_data.hpp
        line_parser.hpp
        line_format.hpp
)

target_link_libraries(line_parsing logging)

target_include_directories(line_parsing
    PUBLIC 
        ${CMAKE_CURRENT_LIST_DIR}
)   