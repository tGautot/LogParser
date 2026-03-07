Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_executable(run_tests
  test_line_format.cpp
  test_line_parser.cpp
  test_line_filter.cpp
  test_filtered_file_reader.cpp
  test_log_parser_interface.cpp
  test_cyclic_deque.cpp
  test_filters.cpp)

target_link_libraries(run_tests
  PRIVATE
    logging
    Catch2::Catch2WithMain
    line_parsing
    file_parsing)
