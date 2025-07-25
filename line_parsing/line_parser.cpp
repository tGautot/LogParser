#include "line_parser.hpp"


Parser::Parser(std::istream& is) : m_is(is){}

void Parser::clearParsingSteps(){
  parsing_routine.clear();
}

void Parser::addParsingStep(parse_instruction_t* inst){
  parsing_routine.push_back(inst);
}

bool Parser::parseLine(std::string& line, ParsedLine* ret){

  std::vector<parse_instruction_t*>::iterator iter;

  char* start_s = line.data();
  char* s = start_s;
  int nint_parsed = 0, ndbl_parsed = 0, nchr_parsed = 0, nstr_parsed = 0;
  for(iter = parsing_routine.begin(); iter != parsing_routine.end(); iter++){
    parse_instruction_t* inst = *iter;
    //std::cout << "pointer is now at char " << *s  << " (offset: " << s-start_s << ")" << std::endl;
    int res = 0;
    switch(inst->ft){
    case FieldType::INT:
      res = inst->parse_func(&s, inst->format_args, (void*) ret->getIntField(nint_parsed++));
      break;
    case FieldType::DBL:
      res = inst->parse_func(&s, inst->format_args, (void*) ret->getDblField(ndbl_parsed++));
      break;
    case FieldType::CHR:
      res = inst->parse_func(&s, inst->format_args, (void*) ret->getChrField(nchr_parsed++));
      break;
    case FieldType::STR:
      res = inst->parse_func(&s, inst->format_args, (void*) ret->getStrField(nstr_parsed++));
      break;
    default:
      res = -1;
    
    }
    if(res != 0) return false;
  }
  return true;

}


