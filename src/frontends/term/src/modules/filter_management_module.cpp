
#include "terminal_modules.hpp"

void FilterManagementModule::registerUserInputMapping(LogParserTerminal&){}
void FilterManagementModule::registerUserActionCallback(LogParserTerminal&) {}
void FilterManagementModule::registerCommandCallback(LogParserTerminal& lpt) {
  lpt.registerCommandCallback([](std::string& cmd, term_state_t& state, LogParserInterface* lpi) -> int{
    // TODO, check for the command you want to handle here (e.g cmd.find(":my_cmd") == 0)
  });
}
