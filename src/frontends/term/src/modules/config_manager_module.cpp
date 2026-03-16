
#include "terminal_modules.hpp"
#include "ConfigHandler.hpp"
#include <filesystem>

void ConfigManagerModule::registerUserInputMapping(LogParserTerminal&){}
void ConfigManagerModule::registerUserActionCallback(LogParserTerminal&) {}
void ConfigManagerModule::registerCommandCallback(LogParserTerminal& lpt) {
  lpt.registerCommandCallback([](std::string& cmd, term_state_t& state, LogParserInterface* lpi) -> int{
    if(cmd.find(":cfg ") != 0) return 0;

    std::string subcmd = cmd.substr(5); // skip ":cfg "

    if(subcmd.find("set ") == 0) {
      std::string kv_str = subcmd.substr(4); // skip "set "
      size_t eq = kv_str.find('=');
      if(eq == std::string::npos) return 1;

      std::string key = kv_str.substr(0, eq);
      std::string val = kv_str.substr(eq + 1);

      ConfigHandler cfg;
      std::string profile = cfg.getProfileForFile(std::filesystem::absolute(lpi->filename).string());
      cfg.set(profile, key, val);
      cfg.save(profile);
      return 1;
    }

    return 1;
  });
}
