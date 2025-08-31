Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_executable(run_tests test.cpp)
target_link_libraries(run_tests 
  PRIVATE 
    logging
    Catch2::Catch2WithMain
    line_parsing
    file_parsing)