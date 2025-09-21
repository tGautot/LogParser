add_executable(lp_term)

target_sources(lp_term
  PRIVATE 
    main.cpp
    src/log_parser_terminal.cpp
    src/modules/cursor_move_module.cpp
    src/modules/arrows_module.cpp
    src/modules/wasd_module.cpp
  PUBLIC 
    include/log_parser_terminal.hpp
    include/lp_terminal_module.hpp
    include/terminal_state.hpp
    include/terminal_modules.hpp)

target_include_directories(lp_term PUBLIC include ../../file_parsing)

target_link_libraries(lp_term logging log_parser)
