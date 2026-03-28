#ifndef LINE_FILTER_HPP
#define LINE_FILTER_HPP

#include "line_format.hpp"
#include "parsing_data.hpp"
#include "processed_line.hpp"

#include <memory>
#include <string>

enum FilterComparison {
  // For all types
  EQUAL, SMALLER, GREATER, GREATER_EQ, SMALLER_EQ,

  // Just for Str
  CONTAINS, BEGINS_WITH, ENDS_WITH
};

enum BitwiseOp {
  AND, OR, XOR, NOR
};

class LineFilter {
  virtual bool _passes(const ProcessedLine* pl) = 0;
  virtual bool _passes(const ParsedLine* pl) = 0;

  bool inverted = false;
public:
  
  void invert(){ inverted = !inverted;}
  bool is_inverted(){ return inverted; }

  bool passes(const ProcessedLine* pl){
    return _passes(pl) ^ inverted;
  }
  bool passes(const ParsedLine* pl){
    return _passes(pl) ^ inverted;
  }
};

class CombinedFilter : public LineFilter {
public:
  std::shared_ptr<LineFilter> left_filter, right_filter;
  BitwiseOp op;
  
  CombinedFilter(std::shared_ptr<LineFilter> left, std::shared_ptr<LineFilter> right, BitwiseOp op);
  
private:
  bool _passes(const ProcessedLine* pl) override;
  bool _passes(const ParsedLine* pl) override;
};

class RawLineFilter : public LineFilter {
public:
  std::string must_contain;
  
  RawLineFilter(std::string& substr);
private:


  bool _passes(const ProcessedLine* pl) override;
  bool _passes(const ParsedLine* pl) override;
};

class FieldFilter : public LineFilter {
private:
  LineField* targetField;
  int parsedFieldId;

  int64_t val_int;
  double val_dbl;
  char val_chr;
  std::string val_str;

  bool case_insensitive_check = false;

  FilterComparison comp;

  
  bool int_passes(const ParsedLine* pl);
  bool dbl_passes(const ParsedLine* pl);
  bool chr_passes(const ParsedLine* pl);
  bool str_passes(const ParsedLine* pl);
  
  bool (FieldFilter::*pass_fn)(const ParsedLine* pl);
public:
  FieldFilter(LineFormat* lfmt, std::string field_name, FilterComparison cmp, std::string& str_value, bool str_case_ins_check = false);
  FieldFilter(LineFormat* lfmt, std::string field_name, FilterComparison cmp, void* base_val, bool str_case_ins_check = false);
private:
  bool _passes(const ProcessedLine* pl) override;
  bool _passes(const ParsedLine* pl) override;

  //bool passes(ParsedLine* pl);
};

class LineNumberFilter : public LineFilter {
public:
  line_t line_from, line_to; 

  LineNumberFilter(line_t from, line_t to);
private:
  bool _passes(const ProcessedLine* pl) override;
  bool _passes(const ParsedLine* pl) override;
};



#endif
