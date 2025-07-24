#include "parser.hpp"


Parser::Parser(std::istream& is) : m_is(is){}

void Parser::clearParsingSteps(){
  parsing_routine.clear();
}

void Parser::addParsingStep(parse_instruction_t* inst){
  parsing_routine.push_back(inst);
}

bool Parser::parseLine(std::string& line/*, ParsedLine* ret*/){

  std::vector<parse_instruction_t*>::iterator iter;

  char* start_s = line.data();
  char* s = start_s;
  int nint_parsed = 0, ndbl_parsed = 0, nchr_parsed = 0, nstr_parsed = 0;
  int64_t pint;
  double pdbl;
  char pchr;
  char* pstr;
  for(iter = parsing_routine.begin(); iter != parsing_routine.end(); iter++){
    parse_instruction_t* inst = *iter;
    //std::cout << "pointer is now at char " << *s  << " (offset: " << s-start_s << ")" << std::endl;
    int res = 0;
    switch(inst->ft){
    case FieldType::INT:
      //std::cout << "Will now parse INT" << std::endl;
      res = inst->parse_func(&s, inst->format_args, (void*) &pint /*ret->getIntField(nint_parsed++)*/);
      std::cout << "Parsed INT (success:" << res << ") got " << pint << std::endl;
      break;
    case FieldType::DBL:
      //std::cout << "Will now parse DBL" << std::endl;
      res = inst->parse_func(&s, inst->format_args, (void*) &pdbl /*ret->getDblField(ndbl_parsed++)*/);
      std::cout << "Parsed DBL (success:" << res << ") got " << pdbl << std::endl;
      break;
    case FieldType::CHR:
      //std::cout << "Will now parse CHR" << std::endl;
      res = inst->parse_func(&s, inst->format_args, (void*) &pchr /*ret->getChrField(nchr_parsed++)*/);
      std::cout << "Parsed CHR (success:" << res << ") got " << pchr << std::endl;
      break;
    case FieldType::STR:
      //std::cout << "Will now parse STR" << std::endl;
      res = inst->parse_func(&s, inst->format_args, (void*) &pstr /*ret->getStrField(nstr_parsed++)*/);
      std::cout << "Parsed STR (success:" << res << ") got " << pstr << std::endl;
      break;
    default:
      res = -1;
    
    }
    if(res != 0) return false;
  }
  return true;

}


