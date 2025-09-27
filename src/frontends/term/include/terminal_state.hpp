#ifndef TERMINAL_STATE_HPP
#define TERMINAL_STATE_HPP

#include <cstdint>
#include <string>
#include <vector>
#include "processed_line.hpp"

enum InputMode {
  ACTION, RAW
};

typedef struct {
  int cy, cx;
  int info_col_size = 0;
  uint64_t line_offset = 0, frame_num = 0;
  uint64_t current_action_multiplier = 1;
  int nrows, ncols;
  InputMode input_mode;
  std::string raw_input;
  bool reached_eof = false;
  std::vector<const ProcessedLine*> displayed_pls;
} term_state_t;

  #endif