#ifndef PARSING_DATA_HPP
#define PARSING_DATA_HPP

#include <stdint.h>
#include <string>
#include <iostream>

typedef struct format_info {
  int nint, ndbl, nchr, nstr;  
} format_info_t;


class ParsedLine {
  public:
  format_info_t& format;

  int64_t* int_fields;
  double* dbl_fields;
  char* chr_fields;
  char** str_fields;

  ParsedLine(format_info_t& fmt);
  ParsedLine(ParsedLine&& old);
  ~ParsedLine();

  int64_t* getIntField(int id){ return int_fields + id; }
  double* getDblField(int id){ return dbl_fields + id; }
  char* getChrField(int id){ return chr_fields + id; }
  char** getStrField(int id){ return str_fields + id; }

  void asStringToStream(std::ostream& os);
};


#endif
