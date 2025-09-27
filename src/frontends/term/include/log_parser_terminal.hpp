#ifndef LOG_PARSER_TERMINAL_HPP
#define LOG_PARSER_TERMINAL_HPP

#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdint>
#include <string>

#include "terminal_state.hpp"
#include "log_parser_interface.hpp"


typedef uint64_t user_action_t;
#define ACTION_NONE 0
using ActionCallbackPtr =  int(*)(user_action_t, term_state_t&, LogParserInterface*);
using CommandCallbackPtr =  int(*)(std::string&, term_state_t&, LogParserInterface*);

class LogParserTerminal {
  
public:
  term_state_t term_state;

  std::string frame_str;
  LogParserInterface* lpi;

  LogParserTerminal(std::string& filename);

  std::vector<std::pair<std::string, user_action_t>> user_input_mappings;
  std::vector<ActionCallbackPtr> action_cbs;
  std::vector<CommandCallbackPtr> command_cbs;


  void registerUserInputMapping(std::string input_seq, user_action_t action_code);
  void registerActionCallback(ActionCallbackPtr action_cb);
  void registerCommandCallback(CommandCallbackPtr command_cb);

  user_action_t getUserAction();
  void handleUserAction(user_action_t action);

  void drawRows();
  void loop();

};

#endif
