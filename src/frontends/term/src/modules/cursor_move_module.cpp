
#include "terminal_modules.hpp"


void CursorMoveModule::registerUserInputMapping(LogParserTerminal& term){
  // Dont register any input, this module only handles move actions
};
void CursorMoveModule::registerUserActionCallback(LogParserTerminal& term){
  term.registerActionCallback([](user_action_t act, term_state_t& term_state, LogParserInterface* _lpi)-> int{
    if(act == ACTION_MOVE_DOWN){
      if(term_state.cy < term_state.nrows - 4*(1-term_state.reached_eof) ){
        term_state.cy++;
      } else {
        if(!term_state.reached_eof) term_state.line_offset++;
      }
    }
    if(act == ACTION_MOVE_UP){
      if(term_state.cy > 4 || (term_state.line_offset == 0 && term_state.cy > 0) ){
        term_state.cy--;
      } else {
        if(term_state.line_offset != 0) {
          term_state.reached_eof = false;
          term_state.line_offset--;
        }
      }
    }
    if(act == ACTION_MOVE_LEFT){
      if(term_state.cx > 0){
        term_state.cx--;
      } else {
        // TODO scroll text left
      }
    }
    if(act == ACTION_MOVE_RIGHT){
      if(term_state.cx < term_state.ncols - 1){
        term_state.cx++;
      } else {
        // TODO scroll text right
      }
    }

    return 0;
  });
};
void CursorMoveModule::registerCommandCallback(LogParserTerminal&){
  
}