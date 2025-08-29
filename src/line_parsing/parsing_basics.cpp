#include "parsing_basics.hpp"
#include <cstdlib>
#include <cstring>

int parse_int(const char** s, int64_t* res){  
  *((int64_t*)res) = atol(*s);
  
  if(*((int64_t*)res) == 0 && **s != '0') return -1;
  while(**s >= '0' && **s <= '9') (*s)++; // Move pointer fwd until char is not digit
  return 0;
}

int parse_dbl(const char** s, double* res){  
  *((double*)res) = atof(*s);

  if(*((double*)res) == 0.0 && **s != '0') return -1;
  int dot_ok = 1;
  while( (**s >= '0' && **s <= '9') || (**s == '.' && dot_ok--)) (*s)++; // Move pointer fwd until char is not digit
  return 0;
}

int parse_chr(const char** s, _ChrFieldOption* p, void* res){

  if(**s == p->target){
    //*(p->res) = p->to_parse;
    *((char*)res) = p->target;
    (*s)++;
  }
  else *((char*)res) = 0;
  while(**s == p->target && p->repeat){
    (*s)++;
  }
  return 0;
}

int parse_str(const char** s, _StrFieldOption* p, void* res){
  //std::cout << "Parsing STR, stop type " << p->stop_type << ", delim " << p->delim << std::endl;
  int nchar = 0;
  if(p->stop_type == StrFieldStopType::NCHAR){
    nchar = p->nchar;
  }
  if(p->stop_type == StrFieldStopType::DELIM){
    while((*s)[nchar] != p->delim && (*s)[nchar] != 0){
      //std::cout << "Now at char " << nchar << ": " << (*s)[nchar] << std::endl; 
      nchar++;
    }
  }
  
  std::string_view* res_sv = (std::string_view*) res;
  *res_sv = std::string_view(*s, nchar);
  *s += nchar;
  return 0;
}
