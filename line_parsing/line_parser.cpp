#include "line_parser.hpp"



Parser* Parser::fromLineFormat(LineFormat* lfmt){
  Parser* p = new Parser();

  LineField* lfield;
  for(int i = 0; i < lfmt->fields.size(); i++){
    lfield = lfmt->fields[i]; 
    if(lfield->ft == FieldType::INT){
      parse_instruction_t* int_parsing = (parse_instruction_t*)malloc(sizeof(parse_instruction_t));;
      int_parsing->parse_func = parse_int;
      int_parsing->format_args = nullptr;
      int_parsing->ft = FieldType::INT;
      p->addParsingStep(int_parsing);
    }
    else if(lfield->ft == FieldType::DBL){
      parse_instruction_t* dbl_parsing = (parse_instruction_t*)malloc(sizeof(parse_instruction_t));;
      dbl_parsing->parse_func = parse_dbl;
      dbl_parsing->format_args = nullptr;
      dbl_parsing->ft = FieldType::DBL;
      p->addParsingStep(dbl_parsing);
    }
    else if(lfield->ft == FieldType::CHR){
      LineChrField* cf = static_cast<LineChrField*>(lfield);
      parse_instruction_t* chr_parsing = (parse_instruction_t*)malloc(sizeof(parse_instruction_t));;
      chr_parsing->parse_func = parse_chr;
      chr_parsing->format_args = (void*) cf->opt;
      chr_parsing->ft = FieldType::CHR;
      p->addParsingStep(chr_parsing);
    }
    else if(lfield->ft == FieldType::STR){
      LineStrField* cf = static_cast<LineStrField*>(lfield);
      parse_instruction_t* str_parsing = (parse_instruction_t*)malloc(sizeof(parse_instruction_t));;
      str_parsing->parse_func = parse_str;
      str_parsing->format_args = (void*) cf->opt;
      str_parsing->ft = FieldType::STR;
      p->addParsingStep(str_parsing);
    }
  }
  return p;
}

Parser::~Parser(){
  for(int i = 0; i < parsing_routine.size(); i++){
    free(parsing_routine[i]);
  }
}

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


