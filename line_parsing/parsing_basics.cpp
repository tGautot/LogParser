#include "parsing_basics.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>

int parse_int(char** s, void* void_params, void* res){
  //parse_int_params_t* p = (parse_int_params_t*) (*void_params);
  //*(p->res) = atol(*s);

  *((int64_t*)res) = atol(*s);

  if(*((int64_t*)res) == 0 && **s != '0') return -1;
  while(**s >= '0' && **s <= '9') (*s)++; // Move pointer fwd until char is not digit
  return 0;
}

int parse_dbl(char** s, void* void_params, void* res){
  //parse_dbl_params_t* p = (parse_dbl_params_t*) (*void_params);
  //*(p->res) = atof(*s);
  
  *((double*)res) = atof(*s);

  if(*((double*)res) == 0.0 && **s != '0') return -1;
  int dot_ok = 1;
  while( (**s >= '0' && **s <= '9') || (**s == '.' && dot_ok--)) (*s)++; // Move pointer fwd until char is not digit
  return 0;
}

int parse_chr(char** s, void* void_params, void* res){
  _ChrFieldOption* p = (_ChrFieldOption*) (void_params);

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

int parse_str(char** s, void* void_params, void* res){
  _StrFieldOption* p = (_StrFieldOption*) (void_params);
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
  //*(p->res) = (char*) malloc(p->nchar * sizeof(char));
  
  *((char**)res) = (char*) malloc((nchar+1) * sizeof(char));
  
  strncpy(*((char**)res), *s, nchar);
  (*((char**)res))[nchar] = 0;
  *s += nchar;
  return 0;
}
