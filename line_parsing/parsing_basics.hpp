#ifndef PARSING_BASICS_HPP
#define PARSING_BASICS_HPP

#include <stdint.h>

enum FieldType {
  INT, DBL, CHR, STR
};

typedef struct parse_instruction {
  int (*parse_func)(char**, void*, void*);
  void** format_args;
  FieldType ft;
} parse_instruction_t;


int parse_int(char** s, void* fmtagrs, void* res);
int parse_dbl(char** s, void* fmtagrs, void* res);
int parse_str(char** s, void* fmtagrs, void* res);
int parse_chr(char** s, void* fmtagrs, void* res);


#define STR_PARSE_STOP_DELIM 0
#define STR_PARSE_STOP_NCHAR 1


/*
typedef struct parse_int_params {
  int64_t* res;
} parse_int_params_t;

typedef struct parse_dbl_params {
  double* res;
} parse_dbl_params_t;
*/

typedef struct parse_str_params{
  int stop_type;

  // For STOP_DELIM
  char delim;

  // for STOP_NCHAR
  int nchar;
  bool skip_eol;

  // To populate result
  //char** res;
} parse_str_params_t;

typedef struct parse_chr_params{
  char to_parse;
  bool do_reapeat;

  //char* res;
} parse_chr_params_t;


#endif

