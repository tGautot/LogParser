#include "parsing_data.hpp"

#include <cstdlib>


ParsedLine::ParsedLine(LineFormat& fmt) : format(fmt){
  int_fields = new int64_t[fmt.nint]; //(int64_t*) malloc(fmt.nint * sizeof(int64_t));
  dbl_fields = new double[fmt.ndbl];//(double*) malloc(fmt.ndbl * sizeof(double));
  chr_fields = new char[fmt.nchr];//(char*) malloc(fmt.nchr * sizeof(char));
  str_fields = new std::string_view[fmt.nstr];//(std::string_view*) calloc(fmt.nstr, sizeof(str_ref));
}

ParsedLine::ParsedLine(ParsedLine&& old) : format(old.format), int_fields(old.int_fields), dbl_fields(old.dbl_fields), chr_fields(old.chr_fields), str_fields(old.str_fields){

  old.int_fields = nullptr;
  old.dbl_fields = nullptr;
  old.chr_fields = nullptr;
  old.str_fields = nullptr;
  
}

ParsedLine::~ParsedLine(){
  
  delete[] int_fields;
  delete[] dbl_fields;
  delete[] chr_fields;
  delete[] str_fields;
}

void ParsedLine::asStringToStream(std::ostream& os){
  int i;
  os << "ParsedLine: ints(";
  for(i = 0; i < format.nint-1; i++) os << *getIntField(i) << ", ";
  if(format.nint >= 1) os << *getIntField(format.nint-1);

  os << "); dbls(";

  for(i = 0; i < format.ndbl-1; i++) os << *getDblField(i) << ", ";
  if(format.ndbl >= 1) os << *getDblField(format.ndbl-1);

  os  << "); chrs(";

  for(i = 0; i < format.nchr-1; i++) os << *getChrField(i) << ", ";
  if(format.nchr >= 1) os << *getChrField(format.nchr-1);

  os  << "); strs(\"";

  for(i = 0; i < format.nstr-1; i++) os << *getStrField(i) << "\", \"";
  if(format.nstr >= 1) os << *getStrField(format.nstr-1);

  os << "\")";
}