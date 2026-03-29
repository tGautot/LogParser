add_library(logging)

target_sources(logging
  PRIVATE
    logging.c
  PUBLIC
    logging.h
    logging.hpp)

target_include_directories(logging PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_library(utils_common)

target_sources(utils_common
  PRIVATE
    string_utils.cpp
  PUBLIC
    string_utils.hpp)

target_include_directories(utils_common PUBLIC ${CMAKE_CURRENT_LIST_DIR})