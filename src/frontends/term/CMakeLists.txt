add_executable(lp_term main.cpp)


target_include_directories(lp_term PUBLIC ../../file_parsing)

target_link_libraries(lp_term log_parser)