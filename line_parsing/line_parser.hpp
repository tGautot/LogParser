#ifndef PARSER_HPP
#define PARSER_HPP

#include "parsing_basics.hpp"
#include "parsing_data.hpp"

#include <iostream>
#include <vector>

class Parser {
public:
  Parser(std::istream& is);
  
  //void set_filter()

  void clearParsingSteps();
  void addParsingStep(parse_instruction_t* inst);

  bool parseLine(std::string& line, ParsedLine* ret);

private:
  std::vector<parse_instruction_t*> parsing_routine;
  std::istream& m_is;
};


#endif