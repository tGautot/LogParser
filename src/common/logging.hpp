#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <cstdint>

extern "C"{
  #include "logging.h"
}
#define LOG_ENTRY(name) LoggerFunctionSentry logger_function_sentry(255, __LINE__, name)
#define LOG_LOGENTRY(lvl, name) LoggerFunctionSentry logger_function_sentry(lvl, __LINE__, name); _log_intrnl(lvl, name, __LINE__, "<ENTRY>\n")
#define LOG_SIMPLE(lvl, fmt, ...) LOG(lvl, fmt, ##__VA_ARGS__)
#define LOG_FCT(lvl, fmt, ...) _log_intrnl(lvl, logger_function_sentry.s.data(), __LINE__, fmt, ##__VA_ARGS__)
#define LOG_EXIT() logger_function_sentry.log_exit(__LINE__)

#include <string>

class LoggerFunctionSentry{
public:
  bool already_exited;
  uint8_t base_lvl; 
  std::string s;
  uint32_t entry_line;
  LoggerFunctionSentry(uint8_t lvl, uint32_t line, std::string name){
    s = std::move(name);
    base_lvl = lvl;
    entry_line = line;
    already_exited = false;
  }
  void log_exit(uint32_t line){
    if(!already_exited && base_lvl != 255){
      _log_intrnl(base_lvl, s.data(), line, "<RETURN>\n");
    }
    already_exited = true;
  }
  ~LoggerFunctionSentry(){
    log_exit(entry_line);
  }
};



#endif