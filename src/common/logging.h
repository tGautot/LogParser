#ifndef LOGGING_H
#define LOGGING_H

#include <stdint.h>
#include <stdarg.h>

void logger_setup();
void logger_teardown();

void logger_set_file(char* filename);

void logger_set_minlvl(uint8_t lvl);


void _log_intrnl(uint8_t lvl, const char* strfile, uint32_t line, const char* fmt, ...);

void _log_raw(uint8_t lvl, const char* fmt, ... );

#define LOG(lvl, fmt, ...) _log_intrnl(lvl, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif