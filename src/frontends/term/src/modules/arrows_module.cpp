#include "terminal_modules.hpp"

void ArrowsModule::registerUserInputMapping(LogParserTerminal& term) {
  term.registerUserInputMapping("\x1b[A", ACTION_MOVE_UP);
  term.registerUserInputMapping("\x1b[B", ACTION_MOVE_DOWN);
  term.registerUserInputMapping("\x1b[C", ACTION_MOVE_RIGHT);
  term.registerUserInputMapping("\x1b[D", ACTION_MOVE_LEFT);
};
void ArrowsModule::registerUserActionCallback(LogParserTerminal&) {
  // This module doesn't handle any action
};
void ArrowsModule::registerCommandCallback(LogParserTerminal&){
  
}