#include "ConfigHandler.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iosfwd>
#include <stdexcept>

#define DEFAULT_PROFILE "common"

#define DEFAULT_CFG_FILE  "[[" DEFAULT_PROFILE "]]\n" \
                          "BG_COL=default\n" \
                          "TXT_COL=default\n" \
                          "LINE_FORMAT={STR:line}\n"

#define CFG_FILE std::string(getenv("HOME")) + "/.lr"

#include "logging.hpp"

ConfigHandler::ConfigHandler(){
  if(kv.size() == 0){
    kv[CFG_BG_COLOR] = "default";
    kv[CFG_TEXT_COLOR] = "default";
    kv[CFG_LINE_FORMAT] = "";
  }
}

bool is_profile_banner(const std::string& line, const std::string& profile){
  LOG(3, "Is %s banner of %s?  pos=%d\n", line.data(), profile.data(), line.find("[[" + profile + "]]"));
  return line.find("[[" + profile + "]]") == 0;
}

void parse_profile(std::ifstream& stream, const std::string& profile){
  LOG_FUNCENTRY(3, "parse_profile");
  LOG_FCT(3, "profile=%s\n", profile.data());
  std::string line;
  while(getline(stream, line)){
    LOG(3, "iter on line=%s\n", line.data());
    if(is_profile_banner(line, profile)){
      // Found the profile we want, look at the kv pairs
      while(getline(stream, line)){
        if(line.find("[[") == 0) break; // Start of next profile, stop
        size_t half = line.find("=");
        if(half == std::string::npos) continue;
        std::string k = line.substr(0, half);
        std::transform(k.begin(), k.end(), k.begin(), ::tolower);
        std::string v = line.substr(half+1);
        LOG(3, "Value of %s is now %s\n", k.data(), v.data());
        kv[k] = v;
      }
      break;

    }
  }
}

void ConfigHandler::load(const std::string& profile){
  LOG_FUNCENTRY(3, "ConfigHandler::load");
  std::ifstream cfs(CFG_FILE);
  if(!cfs.is_open()){
    LOG_FCT(3, "Config file doesn't exit creating it...\n");
    std::ofstream cfg_writer(CFG_FILE);
    if(!cfg_writer.is_open()){
      LOG_FCT(3, "Couldn't create config file, proceeding with default...\n");
      LOG_EXIT();
      return;
    }
    cfg_writer << DEFAULT_CFG_FILE;
    cfg_writer.close();
    LOG_FCT(3, "Config created, re-opening it...\n");
    cfs.open(CFG_FILE);
  }
  LOG_FCT(3, "Config file opened\n");

  std::streampos stt = cfs.tellg();

  std::string line;
  while(getline(cfs, line)){
    LOG_FCT(3, "cfg: %s\n", line.data());
  }
  cfs.clear();
  cfs.seekg(stt); 

  // Need to first load the common profile, then override with specified profile
  parse_profile(cfs, DEFAULT_PROFILE);
  
  cfs.clear();
  cfs.seekg(stt);

  parse_profile(cfs, profile);

  LOG_EXIT();

}

void ConfigHandler::save(const std::string& profile){
  // TODO
}

std::string ConfigHandler::get(const std::string& key){
  if(kv.find(key) == kv.end()){
    throw std::runtime_error("Unknown config key " + key);
  }
  return kv[key];
}

void ConfigHandler::set(const std::string& key, const std::string& val){
  kv[key] = val;
}