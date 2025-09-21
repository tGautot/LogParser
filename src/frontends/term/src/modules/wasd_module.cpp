#include "terminal_modules.hpp"

void WasdModule::registerUserInputMapping(LogParserTerminal& term){
  term.registerUserInputMapping("w", ACTION_MOVE_UP);
  term.registerUserInputMapping("s", ACTION_MOVE_DOWN);
  term.registerUserInputMapping("d", ACTION_MOVE_RIGHT);
  term.registerUserInputMapping("a", ACTION_MOVE_LEFT);
};
void WasdModule::registerUserActionCallback(LogParserTerminal&) {
  // This module doesn't handle any action
};
