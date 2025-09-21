#ifndef TERMINAL_MODULES_HPP
#define TERMINAL_MODULES_HPP

#include "lp_terminal_module.hpp"

class CursorMoveModule : public LogParserTerminalModule {
  public:
  void registerUserInputMapping(LogParserTerminal&) override;
  void registerUserActionCallback(LogParserTerminal&) override;
};
class ArrowsModule : public LogParserTerminalModule {
  public:
  void registerUserInputMapping(LogParserTerminal&) override;
  void registerUserActionCallback(LogParserTerminal&) override;
};
class WasdModule : public LogParserTerminalModule {
  public:
  void registerUserInputMapping(LogParserTerminal&) override;
  void registerUserActionCallback(LogParserTerminal&) override;
};

#endif