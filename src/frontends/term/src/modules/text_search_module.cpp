#include "common.hpp"
#include "log_parser_interface.hpp"
#include "log_parser_terminal.hpp"
#include "terminal_modules.hpp"
#include <cstddef>
#include <utility>

#define ACTION_SEARCH_NEXT 100
#define ACTION_SEARCH_PREV 101

extern "C" {
  #include "logging.h"
}
#include "logging.hpp"

static bool searching = false;
static std::string match_str = "";

static line_t search(term_state_t& state, LogParserInterface* lpi, bool forward){
  // TODO Handle case where cursor in "in the void" and this causes nullptr exception
  line_t cursor_start_line = state.displayed_pls[state.cy]->line_num + (forward ? 1 : 0);
  LOG(3, "Commanding next search to start at line %lu\n", cursor_start_line);
  std::pair<line_t, size_t> match_pos = lpi->findNextOccurence(match_str, cursor_start_line, forward);
  line_t line_num = match_pos.first;
  size_t char_pos = match_pos.second;
  if(line_num == LINE_T_MAX){
    // Dont update state
    // TODO Show user that we found no match
    // Or go around the file
  } else {
    int vert_offset = state.nrows / 2 - 1;
    if(line_num > vert_offset){
      state.line_offset = line_num - vert_offset;
      state.cy = vert_offset;
    } else {
      state.line_offset = 0;
      state.cy = line_num;
    }
    state.cx = char_pos + state.info_col_size;
  }
  return line_num;
}

void TextSearchModule::registerUserInputMapping(LogParserTerminal& term) {
  term.registerUserInputMapping("n", ACTION_SEARCH_NEXT);
  term.registerUserInputMapping("N", ACTION_SEARCH_PREV);
};
void TextSearchModule::registerUserActionCallback(LogParserTerminal& term) {
  term.registerActionCallback([](user_action_t action, term_state_t& state, LogParserInterface* lpi) -> int{
    if(!searching) return 0;
    if(action != ACTION_SEARCH_NEXT && action != ACTION_SEARCH_PREV) return 0;
    search( state, lpi, action == ACTION_SEARCH_NEXT);
    return 0;
  });
};
void TextSearchModule::registerCommandCallback(LogParserTerminal& term){
  term.registerCommandCallback([](std::string& cmd, term_state_t& state, LogParserInterface* lpi) -> int {
    LOG_ENTRY("LAMBDA command callback search");
    size_t substr_pos = cmd.find(":?");
    LOG_FCT(3, "Full command is %s, found search query at pos %lu\n", cmd.data(), substr_pos);
    if(substr_pos == 0){
      searching = true;
      match_str = cmd.substr(2);
      line_t l = search(state, lpi, true);
      LOG_FCT(3, "Command is indeed for search, looking for string %s", match_str.data());
      LOG_FCT(3, "Found it at %lu\n", l);
    } else {
      // TODO maybe searching = false;? See how it feels without any reset
    }
    return 0;
  });
}