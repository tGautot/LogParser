#ifndef PARSING_DATA_HPP
#define PARSING_DATA_HPP

#include "line_format.hpp"

#include <stdint.h>
#include <string>
#include <iostream>



class ParsedLine {
  public:
  LineFormat& format;

  int64_t* int_fields;
  double* dbl_fields;
  char* chr_fields;
  std::string_view* str_fields;

  ParsedLine(LineFormat& fmt);
  ParsedLine(ParsedLine&& old);
  ~ParsedLine();

  int64_t* getIntField(int id){ return int_fields + id; }
  double* getDblField(int id){ return dbl_fields + id; }
  char* getChrField(int id){ return chr_fields + id; }
  std::string_view* getStrField(int id){ return str_fields + id; }

  void asStringToStream(std::ostream& os);
};


#endif
