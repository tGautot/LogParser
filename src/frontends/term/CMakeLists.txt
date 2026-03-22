add_executable(lgm)

target_compile_options(lgm PRIVATE "${CMAKE_CXX_FLAGS_WARNINGS}")

target_sources(lgm
  PRIVATE 
    main.cpp
    src/log_parser_terminal.cpp
    src/modules/cursor_move_module.cpp
    src/modules/arrows_module.cpp
    src/modules/wasd_module.cpp
    src/modules/text_search_module.cpp
    src/modules/vim_quit_module.cpp
    src/modules/filter_management_module.cpp
    src/modules/vim_motions_module.cpp
    src/modules/config_manager_module.cpp
  PUBLIC 
    include/log_parser_terminal.hpp
    include/lp_terminal_module.hpp
    include/terminal_state.hpp
    include/terminal_modules.hpp)

target_include_directories(lgm PUBLIC include ../../file_parsing)

target_link_libraries(lgm logging log_parser)
