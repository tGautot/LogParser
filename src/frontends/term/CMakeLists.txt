add_library(logramtui)

target_compile_options(logramtui PRIVATE "${CMAKE_CXX_FLAGS_WARNINGS}")

target_sources(logramtui
  PRIVATE
    src/log_parser_terminal.cpp
    src/modules/cursor_move_module.cpp
    src/modules/arrows_module.cpp
    src/modules/wasd_module.cpp
    src/modules/text_search_module.cpp
    src/modules/vim_quit_module.cpp
    src/modules/filter_management_module.cpp
    src/modules/vim_motions_module.cpp
    src/modules/config_manager_module.cpp
    src/modules/hide_latest_error_module.cpp
  PUBLIC
    include/log_parser_terminal.hpp
    include/lp_terminal_module.hpp
    include/terminal_state.hpp
    include/terminal_modules.hpp)

target_include_directories(logramtui PUBLIC include ../../file_parsing)

target_link_libraries(logramtui PUBLIC logging log_parser utils_common) 

add_executable(lgm main.cpp)
target_compile_options(lgm PRIVATE "${CMAKE_CXX_FLAGS_WARNINGS}")
target_link_libraries(lgm logramtui)
