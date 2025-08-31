add_library(file_parsing)

target_sources(file_parsing
    PRIVATE 
        log_parser_interface.cpp 
        filtered_file_reader.cpp  
        processed_line.cpp
        line_filter.cpp
    PUBLIC 
        log_parser_interface.hpp
        filtered_file_reader.hpp
        processed_line.hpp
        line_filter.hpp
        common.hpp
)

target_include_directories(file_parsing
    PUBLIC 
        ${CMAKE_CURRENT_LIST_DIR}
)  

target_link_libraries(file_parsing
    line_parsing
    logging)