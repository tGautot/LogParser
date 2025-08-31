add_library(logging SHARED)

target_sources(logging
  PRIVATE
    logging.c
  PUBLIC
    logging.h)

target_include_directories(logging PUBLIC ${CMAKE_CURRENT_LIST_DIR})