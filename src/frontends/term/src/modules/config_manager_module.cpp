
#include "terminal_modules.hpp"

void ConfigManagerModule::registerUserInputMapping(LogParserTerminal&){}
void ConfigManagerModule::registerUserActionCallback(LogParserTerminal&) {}
void ConfigManagerModule::registerCommandCallback(LogParserTerminal& lpt) {
  lpt.registerCommandCallback([](std::string& cmd, term_state_t& state, LogParserInterface* lpi) -> int{
    // TODO, check for the command you want to handle here (e.g cmd.find(":my_cmd") == 0)
    if(cmd.find(":cfg ") != 0) return 0;
    
    return 0;
  });
}
