#include "parsing_data.hpp"

#include <cstdlib>

ParsedLine::ParsedLine(format_info_t& fmt) : format(fmt){
  int_fields = (int64_t*) malloc(fmt.nint * sizeof(int64_t));
  dbl_fields = (double*) malloc(fmt.ndbl * sizeof(double));
  chr_fields = (char*) malloc(fmt.nchr * sizeof(char));
  str_fields = (char**) malloc(fmt.nstr * sizeof(char*));
}

ParsedLine::ParsedLine(ParsedLine&& old) : format(old.format), int_fields(old.int_fields), dbl_fields(old.dbl_fields), chr_fields(old.chr_fields), str_fields(old.str_fields){

  old.int_fields = nullptr;
  old.dbl_fields = nullptr;
  old.chr_fields = nullptr;
  old.str_fields = nullptr;
  
}

ParsedLine::~ParsedLine(){
  free(int_fields);
  free(dbl_fields);
  free(chr_fields);
  free(str_fields);
  for(int i = 0; i < format.nstr; i++){
    if(str_fields[i] != nullptr) free(str_fields[i]);
  }
}

