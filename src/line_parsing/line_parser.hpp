#ifndef PARSER_HPP
#define PARSER_HPP

#include "parsing_basics.hpp"
#include "parsing_data.hpp"

#include <vector>
#include <memory>

class Parser {
public:
  
  static std::shared_ptr<Parser> fromLineFormat(std::unique_ptr<LineFormat> lf);
  ~Parser();

  void addParsingStep(parse_instruction_t* inst);

  bool parseLine(std::string_view line, ParsedLine* ret);

  std::unique_ptr<LineFormat> format;
private:
  std::vector<parse_instruction_t*> parsing_routine;
};


#endif