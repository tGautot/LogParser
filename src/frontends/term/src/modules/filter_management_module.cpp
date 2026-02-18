
#include "line_filter.hpp"
#include "terminal_modules.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <stdio.h>


// Trim from the start (in place)
inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// Trim from the end (in place)
inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

inline void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}

#define CHECK_FOR_TAG(cmp_txt, code_op) \
  v = s.find_first_of(cmp_txt, start_pos); \
  if(v < ans){ \
    ans = v; op = code_op; \
  } 

std::pair<size_t, BitwiseOp> find_next_bitwise_op(std::string& s, size_t start_pos=0){
  BitwiseOp op = AND;
  size_t ans = std::string::npos;
  size_t v = 0;
  CHECK_FOR_TAG("AND", AND);
  CHECK_FOR_TAG("OR", OR);
  CHECK_FOR_TAG("XOR", XOR);
  CHECK_FOR_TAG("NOR", NOR);
  return {ans, op};
}



std::pair<size_t, FilterComparison> find_next_comparator(std::string& s, size_t start_pos=0){
  FilterComparison op = EQUAL;
  size_t ans = std::string::npos;
  size_t v = 0;
  CHECK_FOR_TAG("EQ", EQUAL);
  CHECK_FOR_TAG("ST", SMALLER);
  CHECK_FOR_TAG("SE", SMALLER_EQ);
  CHECK_FOR_TAG("GT", GREATER);
  CHECK_FOR_TAG("GE", GREATER_EQ);
  CHECK_FOR_TAG("CT", CONTAINS);
  CHECK_FOR_TAG("BW", BEGINS_WITH);
  CHECK_FOR_TAG("EW", ENDS_WITH);
  return {ans, op};
}

std::shared_ptr<LineFilter> parse_filter_decl(std::string fdecl, LineFormat* lfmt){
parse_start:
  trim(fdecl);
  if(fdecl[0] == '('){
    size_t expr_end = 0;
    size_t open_prt = 0;
    do {
      if(fdecl[expr_end] == '(') open_prt++;
      else if(fdecl[expr_end] == ')') open_prt--;
    } while(open_prt > 0 && ++expr_end < fdecl.size());
    if(expr_end == fdecl.size()) throw std::runtime_error("Found '(' but no matching ')'");
    if(expr_end+1 == fdecl.size()){
      fdecl = (fdecl.substr(1, fdecl.size()-2));
      goto parse_start;
    }

    std::shared_ptr<LineFilter> left = parse_filter_decl(fdecl.substr(1, expr_end-1), lfmt);

    std::pair<size_t, BitwiseOp> op_stt = find_next_bitwise_op(fdecl, expr_end);

    size_t offset = 3;
    if(op_stt.second == OR) offset = 2;
    std::shared_ptr<LineFilter> right = parse_filter_decl(fdecl.substr(op_stt.first + offset), lfmt);
  
    return std::make_shared<CombinedFilter>(left, right, op_stt.second);
  } else {
    std::pair<size_t, BitwiseOp> op_stt = find_next_bitwise_op(fdecl);
    if(op_stt.first != std::string::npos){
      std::shared_ptr<LineFilter> left = parse_filter_decl(fdecl.substr(0, op_stt.first), lfmt);

      size_t offset = 3;
      if(op_stt.second == OR) offset = 2;
      std::shared_ptr<LineFilter> right = parse_filter_decl(fdecl.substr(op_stt.first + offset), lfmt);
    
      return std::make_shared<CombinedFilter>(left, right, op_stt.second);
    }

    // We are now supposed to only have a simple field filter (e.g.: "field_name COMPARATOR value")
    std::pair<size_t, FilterComparison> cmp_stt = find_next_comparator(fdecl);
    std::string field_name = fdecl.substr(0, cmp_stt.first); trim(field_name);
    std::string value_str = fdecl.substr(cmp_stt.first+2); trim(value_str);

    return std::make_shared<FieldFilter>(lfmt, field_name, cmp_stt.second, value_str);
  }
}

void FilterManagementModule::registerUserInputMapping(LogParserTerminal&){}
void FilterManagementModule::registerUserActionCallback(LogParserTerminal&) {}
void FilterManagementModule::registerCommandCallback(LogParserTerminal& lpt) {
  lpt.registerCommandCallback([](std::string& full_cmd, term_state_t& state, LogParserInterface* lpi) -> int{
    if(full_cmd.find(":fclear") == 0){
      lpi->clearFilter();
      return 0;
    }

    LineFormat* lf = lpi->getLineFormat() ;
    if(lf == nullptr){
      throw std::runtime_error("Cannot set format without having specified a line format");
    }
    size_t space_pos = full_cmd.find(" ");
    std::string cmd = full_cmd.substr(0, space_pos);

    if(cmd == ":fset" || cmd == ":f" || cmd == ":fadd" || cmd == ":fand" ||
        cmd == ":for" || cmd == ":fxor" || cmd == ":fnor" || cmd == ":fout"){
      
      std::string filter_str = "";

      size_t arg_start = space_pos; 
      while(arg_start < cmd.size() && std::isspace(cmd[arg_start])){arg_start++;};
      if(arg_start < cmd.size()){
        std::ifstream file(cmd.data() + arg_start);
        if(file.is_open() && file.good()){
          getline(file, filter_str);
        } else {
          filter_str = cmd.data() + arg_start;
        }
      } else {
        throw std::runtime_error("Expecting a second argument");
      }

      std::shared_ptr<LineFilter> filter = parse_filter_decl(filter_str, lf);


      if(cmd.find(":fset ") == 0){
        lpi->setFilter(filter);
      }
      
      
      
      std::shared_ptr<LineFilter> old_filter = lpi->getFilter();
      if(old_filter == nullptr) {
        lpi->setFilter(filter);
        return 0;
      }
      std::shared_ptr<LineFilter> new_filter;
      if(cmd.find(":f ") == 0 || cmd.find(":fadd ") == 0 || cmd.find(":fand ") == 0){
        new_filter = std::make_shared<CombinedFilter>(old_filter, filter, AND);
      }else if(cmd.find(":for ") == 0){
        new_filter = std::make_shared<CombinedFilter>(old_filter, filter, OR);
      } else if(cmd.find(":fxor ") == 0){
        new_filter = std::make_shared<CombinedFilter>(old_filter, filter, XOR);
      } else if(cmd.find(":fnor ") == 0){
        new_filter = std::make_shared<CombinedFilter>(old_filter, filter, NOR);
      } else if(cmd.find(":fout ") == 0){
        // Filter OUT, we don't want to see the lines passing that filter
        filter->invert();
        new_filter = std::make_shared<CombinedFilter>(old_filter, filter, AND);
      }
      lpi->setFilter(filter);
    }    
    return 0;
  });
}
