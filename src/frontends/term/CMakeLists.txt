add_executable(lp_term)

target_sources(lp_term
  PRIVATE 
    main.cpp
  PUBLIC 
    include/log_parser_terminal.hpp
    include/lp_terminal_module.hpp)

target_include_directories(lp_term PUBLIC ../../file_parsing)

target_link_libraries(lp_term log_parser logging)