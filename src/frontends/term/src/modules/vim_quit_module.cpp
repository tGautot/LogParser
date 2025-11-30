#include "terminal_modules.hpp"

#include <string>

void VimQuitModule::registerUserInputMapping(LogParserTerminal&){}
void VimQuitModule::registerUserActionCallback(LogParserTerminal&) {}
void VimQuitModule::registerCommandCallback(LogParserTerminal& lpt) {
  lpt.registerCommandCallback([](std::string& command) -> int{
    if(command == "q") {
      exit(0);
    }
  })
}