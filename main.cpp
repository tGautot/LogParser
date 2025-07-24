#include <iostream>
#include <fstream>
#include <string>

#include "parser.hpp"

parse_instruction_t* parseStrUntilChar(char delim){
  parse_instruction_t* str_parsing = (parse_instruction_t*)malloc(sizeof(parse_instruction_t));
  parse_str_params_t* parse_params = (parse_str_params_t*)malloc(sizeof(parse_str_params_t));
  parse_params->delim = delim;
  parse_params->skip_eol = false;
  parse_params->stop_type = STR_PARSE_STOP_DELIM;

  str_parsing->format_args = (void**) parse_params;
  str_parsing->parse_func = parse_str;
  str_parsing->ft = FieldType::STR;
  return str_parsing;
}

parse_instruction_t* parseChar(char c, bool repeat){
  parse_instruction_t* chr_parsing = (parse_instruction_t*)malloc(sizeof(parse_instruction_t));
  parse_chr_params_t* parse_params = (parse_chr_params_t*)malloc(sizeof(parse_chr_params_t));
  parse_params->do_reapeat = repeat;
  parse_params->to_parse = c;

  chr_parsing->format_args = (void**) parse_params;
  chr_parsing->parse_func = parse_chr;
  chr_parsing->ft = FieldType::CHR;
  return chr_parsing;
}




int main(int argc, char** argv){
  if(argc == 1){
    std::cout << "Specify a file" << std::endl;
    return 1;
  }

  parse_instruction_t* int_parsing = (parse_instruction_t*)malloc(sizeof(parse_instruction_t));;
  int_parsing->parse_func = parse_int;
  int_parsing->format_args = nullptr;
  int_parsing->ft = FieldType::INT;

  parse_instruction_t* whitespace_parsing = parseChar(' ', true);
  
  parse_instruction_t* single_colon_parsing = parseChar(':', false);

  parse_instruction_t* dot_parsing = parseChar('.', true);
  
  parse_instruction_t* str_to_ws_parsing = parseStrUntilChar(' ');
  
  parse_instruction_t* str_to_colon_parsing = parseStrUntilChar(':');

  parse_instruction_t* str_to_eol_parsing = parseStrUntilChar(0);

  Parser parser(std::cin);
  parser.addParsingStep(int_parsing);
  parser.addParsingStep(whitespace_parsing);
  parser.addParsingStep(int_parsing);
  parser.addParsingStep(whitespace_parsing);
  parser.addParsingStep(str_to_ws_parsing);
  parser.addParsingStep(whitespace_parsing);
  parser.addParsingStep(single_colon_parsing);
  parser.addParsingStep(dot_parsing);
  parser.addParsingStep(str_to_colon_parsing);
  parser.addParsingStep(single_colon_parsing);
  parser.addParsingStep(str_to_eol_parsing);



  std::ifstream file(argv[1]);
  std::string line;
  while(std::getline(file, line)){
    parser.parseLine(line);
  }




}
