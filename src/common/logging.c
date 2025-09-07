#include "logging.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define FILE_SEP '/'  

#if defined(_WIN32) || defined(WIN32)     /* _Win32 is usually defined by compilers targeting 32 or   64 bit Windows systems */
#undef FILE_SEP
#define FILE_SEP '\\'
#endif


static struct {
  FILE*  fp;
  uint8_t minlvl;
} config = {NULL, 0};

void logger_set_file(char* filename){
  FILE* fp = fopen(filename, "w+");
  if(fp == NULL){
    config.fp = stdout;
  } else {
    config.fp = fp;
  }
}

void logger_setup(){
  config.fp = stdout;
}
void logger_teardown(){
  if(config.fp != NULL && config.fp != stdout){
    fclose(config.fp);
  }
}

void logger_to_stdout(){
  config.fp = stdout;
}

void logger_set_minlvl(uint8_t lvl){
  config.minlvl = lvl;
}


void _log_intrnl(uint8_t lvl, const char* strfile, uint32_t line, const char* fmt, ...){
  if(lvl < config.minlvl) return;
  if(lvl > 9) lvl = 9;

  time_t rawtime;
  struct tm * timeinfo;
  char timstr[80];
  
  time (&rawtime);
  timeinfo = localtime(&rawtime);
  
  strftime(timstr,sizeof(timstr),"%d-%m-%Y %H:%M:%S",timeinfo);
  
  int i = 0;
  const char* file_stt = strfile;
  while(strfile[i++] != 0){
    if(strfile[i-1] == FILE_SEP){
      file_stt = strfile + i;
    }
  }
  char* final_fmt = (char*) malloc(strlen(timstr) + strlen(file_stt) + (line/10 + 1)  + strlen(fmt) + 20);
  sprintf(final_fmt, "%s [%s:%d:%d] %s", timstr, file_stt, line, lvl, fmt);
  
  va_list lst;
  va_start(lst, fmt);
  vfprintf(config.fp, final_fmt, lst);
  va_end(lst);
  
  free(final_fmt);
}

void _log_raw(uint8_t lvl, const char* fmt, ... ){
  va_list lst;
  va_start(lst, fmt);
  
  vfprintf(config.fp, fmt, lst);
  va_end(lst);
}
