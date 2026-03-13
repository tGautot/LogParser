add_library(config_handler)

target_compile_options(config_handler PRIVATE "${CMAKE_CXX_FLAGS_WARNINGS}")

target_sources(config_handler
  PRIVATE
    ConfigHandler.cpp
  PUBLIC
    ConfigHandler.hpp
)

target_include_directories(config_handler
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(config_handler
  PRIVATE
    logging)