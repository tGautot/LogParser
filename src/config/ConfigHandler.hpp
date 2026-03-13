#ifndef CONFIG_HANDLER_HPP
#define CONFIG_HANDLER_HPP

#include <map>
#include <string>

#define CFG_BG_COLOR "bg_col"
#define CFG_TEXT_COLOR "txt_col"
#define CFG_LINE_FORMAT "line_format"

// Only one config allowed at runtime, make kv static so that all instances share it
static std::map<std::string, std::string> kv;

class ConfigHandler {
public: 
  ConfigHandler();
  
  void load(const std::string& profile); 
  void save(const std::string& profile);

  std::string get(const std::string& key);
  void set(const std::string& key, const std::string& val);
};



#endif


