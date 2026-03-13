#include <memory>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>

#include <string>

#include "line_format.hpp"
#include "log_parser_terminal.hpp"
#include "ConfigHandler.hpp"

#include "logging.hpp"


#define CTRL_CHR(c) (c & 0x1f)

#define ESC_CHR '\x1b'
#define ESC_CMD "\x1b["

#define CLEAR_SCREEN() write(STDOUT_FILENO, ESC_CMD "2J", 4);
#define CURSOR_TL() write(STDOUT_FILENO, ESC_CMD "H", 3);

#include "terminal_modules.hpp"

#include <csignal>

void signal_handler(int signal)
{
    logger_teardown();
    exit(1);
}
 

int main(int argc, char** argv){
  logger_setup();
  std::signal(SIGINT, signal_handler);
  logger_set_file("./logs.log");
  logger_set_minlvl(5);
  LOG(3, "Starting log\n");
  if(argc != 2){
    printf("Usage ./lp_term <file_path>\n");
    return 1;
  }
  std::string filepath = argv[1];
  size_t slash_pos = filepath.find_last_of('/');
  std::string filename = filepath;
  if(slash_pos != std::string::npos){
    filename = filepath.substr(slash_pos+1);
  }


  ConfigHandler cfg;
  // Profile based on filename, not full path. Is a choice, might change
  cfg.load(filename);
  std::string format_spec = cfg.get(CFG_LINE_FORMAT);
  std::unique_ptr<LineFormat> line_format = LineFormat::fromFormatString(format_spec);

  LogParserTerminal lpt(filepath, std::move(line_format));
  lpt.config.bg_col = cfg.get(CFG_BG_COLOR);
  lpt.config.txt_col = cfg.get(CFG_TEXT_COLOR);
  // TODO load modules on the fly based on config
  // dlopen is fun, but should probably make python/lua bindings at some points
  CursorMoveModule cmm;
  cmm.registerUserInputMapping(lpt);
  cmm.registerUserActionCallback(lpt);
  
  ArrowsModule am;
  am.registerUserInputMapping(lpt);
  am.registerUserActionCallback(lpt);
  
  WasdModule wm;
  wm.registerUserInputMapping(lpt);
  wm.registerUserActionCallback(lpt);
  
  TextSearchModule tsm;
  tsm.registerUserInputMapping(lpt);
  tsm.registerUserActionCallback(lpt);
  tsm.registerCommandCallback(lpt);

  VimQuitModule vqm;
  vqm.registerUserInputMapping(lpt);
  vqm.registerUserActionCallback(lpt);
  vqm.registerCommandCallback(lpt);


  FilterManagementModule fmm;
  fmm.registerUserInputMapping(lpt);
  fmm.registerUserActionCallback(lpt);
  fmm.registerCommandCallback(lpt);

  VimMotionsModule vmm;
  vmm.registerUserInputMapping(lpt);
  vmm.registerUserActionCallback(lpt);
  vmm.registerCommandCallback(lpt);

  ConfigManagerModule cfgmm;
  cfgmm.registerUserInputMapping(lpt);
  cfgmm.registerUserActionCallback(lpt);
  cfgmm.registerCommandCallback(lpt);
  lpt.loop();
  logger_teardown();
}

