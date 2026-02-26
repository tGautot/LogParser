
#include "lp_terminal_module.hpp"
#include "terminal_modules.hpp"
#include <cstdlib>

#define ACTION_GO_TO_FILE_BEGINNING 200
#define ACTION_GO_TO_FILE_END 201

#define is_digit(c) (c >= '0' && c <= '9')

void VimMotionsModule::registerUserInputMapping(LogParserTerminal& lpt){
  lpt.registerUserInputMapping("gg", ACTION_GO_TO_FILE_BEGINNING);
  lpt.registerUserInputMapping("G", ACTION_GO_TO_FILE_END);
  lpt.registerUserInputMapping("h", ACTION_MOVE_LEFT);
  lpt.registerUserInputMapping("j", ACTION_MOVE_DOWN);
  lpt.registerUserInputMapping("k", ACTION_MOVE_UP);
  lpt.registerUserInputMapping("l", ACTION_MOVE_RIGHT);
}
void VimMotionsModule::registerUserActionCallback(LogParserTerminal& lpt) {
  lpt.registerActionCallback([](user_action_t act, term_state_t& term_state, LogParserInterface* lpi)-> int{
    if(act == ACTION_GO_TO_FILE_BEGINNING){
      term_state.cy = 0;
      term_state.line_offset = 0;
    }
    if(act == ACTION_GO_TO_FILE_END){

    }
    return 0;
  });
}
void VimMotionsModule::registerCommandCallback(LogParserTerminal& lpt) {
  lpt.registerCommandCallback([](std::string& cmd, term_state_t& state, LogParserInterface* lpi) -> int{
    if(cmd.size() == 1 || !is_digit(cmd[1])) {
      return 1;
    }
    /* 
      TODO: Allow using :1234 to go to line 1234
      Need to define if this would be local or global line id, needs clarity
    
    int line_num = atoi(cmd.data() + 1);
    line_t lpi_fl = lpi->block.first_line_local_id;
    line_t lpi_ll = lpi->block.last_line_glbl_id;
    if(line_num < lpi_fl){

    } else if(line_num > lpi_ll){

    } else {
      // Is already in block, 
    }*/
    return 0;
  });
}
