#ifndef LINE_FILTER_HPP
#define LINE_FILTER_HPP

#include "line_format.hpp"
#include "parsing_data.hpp"

#include <string>

enum FilterComparison {
  // For all types
  EQUAL, SMALLER, GREATER, GREATER_EQ, SMALLER_EQ,

  // Just for Str
  CONTAINS, BEGINS_WITH, ENDS_WITH
};

class LineFilter {
private:
  LineField* targetField;
  int parsedFieldId;

  int64_t val_int;
  double val_dbl;
  char val_chr;
  std::string val_str;

  FilterComparison comp;
  bool neg;

  
  bool int_passes(ParsedLine* pl);
  bool dbl_passes(ParsedLine* pl);
  bool chr_passes(ParsedLine* pl);
  bool str_passes(ParsedLine* pl);
  
  bool (LineFilter::*pass_fn)(ParsedLine* pl);
  public:
  
  LineFilter(LineFormat* lfmt, std::string field_name, FilterComparison cmp, void* base_val, bool neg = false);
  bool passes(ParsedLine* pl);

  //bool passes(ParsedLine* pl);
};



#endif
