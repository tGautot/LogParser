add_library(logging)

target_sources(logging
  PRIVATE
    logging.c
  PUBLIC
    logging.h
    logging.hpp)

target_include_directories(logging PUBLIC ${CMAKE_CURRENT_LIST_DIR})