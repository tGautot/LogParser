#include "line_parser.hpp"
#include <memory>


std::shared_ptr<Parser> Parser::fromLineFormat(std::unique_ptr<LineFormat> lfmt){
  std::shared_ptr<Parser> p = std::make_shared<Parser>();
  if(lfmt == nullptr) return p;
  LineField* lfield;
  for(size_t i = 0; i < lfmt->fields.size(); i++){
    lfield = lfmt->fields[i]; 
    if(lfield->ft == FieldType::INT){
      parse_instruction_t* int_parsing = (parse_instruction_t*)malloc(sizeof(parse_instruction_t));;
      int_parsing->format_args = nullptr;
      int_parsing->ft = FieldType::INT;
      p->addParsingStep(int_parsing);
    }
    else if(lfield->ft == FieldType::DBL){
      parse_instruction_t* dbl_parsing = (parse_instruction_t*)malloc(sizeof(parse_instruction_t));;
      dbl_parsing->format_args = nullptr;
      dbl_parsing->ft = FieldType::DBL;
      p->addParsingStep(dbl_parsing);
    }
    else if(lfield->ft == FieldType::CHR){
      LineChrField* cf = static_cast<LineChrField*>(lfield);
      parse_instruction_t* chr_parsing = (parse_instruction_t*)malloc(sizeof(parse_instruction_t));;
      chr_parsing->format_args = (void*) cf->opt;
      chr_parsing->ft = FieldType::CHR;
      p->addParsingStep(chr_parsing);
    }
    else if(lfield->ft == FieldType::STR){
      LineStrField* cf = static_cast<LineStrField*>(lfield);
      parse_instruction_t* str_parsing = (parse_instruction_t*)malloc(sizeof(parse_instruction_t));;
      str_parsing->format_args = (void*) cf->opt;
      str_parsing->ft = FieldType::STR;
      p->addParsingStep(str_parsing);
    }
  }
  p->format = std::move(lfmt);
  return p;
}

Parser::~Parser(){
  for(size_t i = 0; i < parsing_routine.size(); i++){
    free(parsing_routine[i]);
  }
}

void Parser::clearParsingSteps(){
  parsing_routine.clear();
}

void Parser::addParsingStep(parse_instruction_t* inst){
  parsing_routine.push_back(inst);
}

#include "logging.hpp"

bool Parser::parseLine(std::string_view line, ParsedLine* ret){
  //LOG(1, "Going to parse line with format %p\n", format.get());
  if(format == nullptr) return false;
  std::vector<parse_instruction_t*>::iterator iter;

  const char* s = line.data();
  int nint_parsed = 0, ndbl_parsed = 0, nchr_parsed = 0, nstr_parsed = 0;
  //LOG(1, "Going into loop!!!!!!!!!e\n");
  for(iter = parsing_routine.begin(); iter != parsing_routine.end(); iter++){
    parse_instruction_t* inst = *iter;
    //LOG(1, "Parsing new step, field type=%d\n", inst->ft);
    //std::cout << "pointer is now at char " << *s  << " (offset: " << s-line.data() << ")" << std::endl;
    int res = 0;
    switch(inst->ft){
    case FieldType::INT:
      res = parse_int(&s, ret->getIntField(nint_parsed++));
      break;
    case FieldType::DBL:
      res = parse_dbl(&s, ret->getDblField(ndbl_parsed++));
      break;
    case FieldType::CHR:
      res = parse_chr(&s, (_ChrFieldOption*) inst->format_args, ret->getChrField(nchr_parsed++));
      break;
    case FieldType::STR:
      res = parse_str(&s, (_StrFieldOption*) inst->format_args, ret->getStrField(nstr_parsed++));
      break;
    default:
      res = -1;
    }
    if(res != 0) return false;
  }
  return true;

}


