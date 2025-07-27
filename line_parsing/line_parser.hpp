#ifndef PARSER_HPP
#define PARSER_HPP

#include "parsing_basics.hpp"
#include "parsing_data.hpp"

#include <iostream>
#include <vector>

class Parser {
public:
  
  static Parser* fromLineFormat(LineFormat* lf);
  ~Parser();

  //void set_filter()

  void clearParsingSteps();
  void addParsingStep(parse_instruction_t* inst);

  bool parseLine(std::string_view line, ParsedLine* ret);

private:
  std::vector<parse_instruction_t*> parsing_routine;
};


#endif