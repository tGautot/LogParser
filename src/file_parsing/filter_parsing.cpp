
#include "filter_parsing.hpp"
#include "line_filter.hpp"
#include "line_format.hpp"
#include "string_utils.hpp"
#include "logging.hpp"

#include <algorithm>
#include <cctype>
#include <memory>
#include <stdexcept>
#include <string>

#define CHECK_FOR_TAG(cmp_txt, code_op) \
  v = s.find(" " cmp_txt " ", start_pos); \
  if(v < ans){ \
    ans = v+1; op = code_op; \
  }

#define CHECK_FOR_COMPARATOR_TAG(cmp_txt, code_op) \
  v = s.find(" " cmp_txt " ", start_pos); \
  if(v < tag_stt_pos){  \
    tag_size = sizeof(cmp_txt)-1; \
    case_ins_check = false; \
    tag_stt_pos = v+1; op = code_op; \
    LOG(7, "Found tag %s (size: %d) at pos %d\n", cmp_txt, tag_size, v); \
  } else if( (v = s.find(" " cmp_txt "_CI ", start_pos)) < tag_stt_pos ) { \
    tag_size = sizeof(cmp_txt)-1+3; \
    case_ins_check = true; \
    tag_stt_pos = v+1; op = code_op; \
    LOG(7, "Found tag %s_CI (size: %d) at pos %d\n", cmp_txt, tag_size, v); \
  }

std::pair<size_t, BitwiseOp> find_next_bitwise_op(std::string& s, size_t start_pos){
  BitwiseOp op = AND;
  size_t ans = std::string::npos;
  size_t v = 0;
  CHECK_FOR_TAG("AND", AND);
  CHECK_FOR_TAG("OR", OR);
  CHECK_FOR_TAG("XOR", XOR);
  CHECK_FOR_TAG("NOR", NOR);
  return {ans, op};
}


// Tag start pos, tag size, comparison, is_case_insensitive
std::tuple<size_t, size_t, FilterComparison, bool> find_next_comparator(std::string& s, size_t start_pos){
  LOG(3, "Looking for tag in \"%s\"\n", s.data());
  FilterComparison op = EQUAL;
  size_t tag_stt_pos = std::string::npos;
  size_t tag_size = std::string::npos;
  size_t v = 0;
  bool case_ins_check = false;

  CHECK_FOR_COMPARATOR_TAG("EQ", EQUAL);
  CHECK_FOR_COMPARATOR_TAG("EQUAL", EQUAL);

  CHECK_FOR_COMPARATOR_TAG("ST", SMALLER);
  CHECK_FOR_COMPARATOR_TAG("SMALLER", SMALLER);
  CHECK_FOR_COMPARATOR_TAG("SMALLER_THAN", SMALLER);

  CHECK_FOR_COMPARATOR_TAG("SE", SMALLER_EQ);
  CHECK_FOR_COMPARATOR_TAG("SMALLER_EQ", SMALLER_EQ);
  CHECK_FOR_COMPARATOR_TAG("SMALLER_EQUAL", SMALLER_EQ);
  CHECK_FOR_COMPARATOR_TAG("SMALLER_OR_EQUAL", SMALLER_EQ);

  CHECK_FOR_COMPARATOR_TAG("GT", GREATER);
  CHECK_FOR_COMPARATOR_TAG("GREATER", GREATER);
  CHECK_FOR_COMPARATOR_TAG("GREATER_THAN", GREATER);

  CHECK_FOR_COMPARATOR_TAG("GE", GREATER_EQ);
  CHECK_FOR_COMPARATOR_TAG("GREATER_EQ", GREATER_EQ);
  CHECK_FOR_COMPARATOR_TAG("GREATER_EQUAL", GREATER_EQ);
  CHECK_FOR_COMPARATOR_TAG("GREATER_OR_EQUAl", GREATER_EQ);

  CHECK_FOR_COMPARATOR_TAG("CT", CONTAINS);
  CHECK_FOR_COMPARATOR_TAG("CONTAINS", CONTAINS);

  CHECK_FOR_COMPARATOR_TAG("BW", BEGINS_WITH);
  CHECK_FOR_COMPARATOR_TAG("BEGINS_WITH", BEGINS_WITH);

  CHECK_FOR_COMPARATOR_TAG("SW", BEGINS_WITH);
  CHECK_FOR_COMPARATOR_TAG("STARTS_WITH", BEGINS_WITH);

  CHECK_FOR_COMPARATOR_TAG("EW", ENDS_WITH);
  CHECK_FOR_COMPARATOR_TAG("ENDS_WITH", ENDS_WITH);

  return {tag_stt_pos, tag_size, op, case_ins_check};
}

std::shared_ptr<LineFilter> parse_filter_decl(std::string fdecl, LineFormat* lfmt){
parse_start:
  LOG(3, "Parsing %s\n", fdecl.data());
  if(fdecl == "") return nullptr;
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
      LOG(3, "Found global matching parenthesis, removing and restarting parse\n");
      fdecl = (fdecl.substr(1, fdecl.size()-2));
      goto parse_start;
    }

    LOG(3, "Found parenthesis at pos %d, %d, parsing content: \"%s\"\n", 1, expr_end-1, fdecl.substr(1, expr_end-1).data());
    std::shared_ptr<LineFilter> left = parse_filter_decl(fdecl.substr(1, expr_end-1), lfmt);

    std::pair<size_t, BitwiseOp> op_stt = find_next_bitwise_op(fdecl, expr_end);

    size_t offset = 3;
    if(op_stt.second == OR) offset = 2;
    std::shared_ptr<LineFilter> right = parse_filter_decl(fdecl.substr(op_stt.first + offset), lfmt);

    return std::make_shared<CombinedFilter>(left, right, op_stt.second);
  } else {
    std::pair<size_t, BitwiseOp> op_stt = find_next_bitwise_op(fdecl);
    LOG(3, "Found bitwise op at pos %lu\n", op_stt.first);
    if(op_stt.first != std::string::npos){
      std::shared_ptr<LineFilter> left = parse_filter_decl(fdecl.substr(0, op_stt.first), lfmt);

      size_t offset = 3;
      if(op_stt.second == OR) offset = 2;
      std::shared_ptr<LineFilter> right = parse_filter_decl(fdecl.substr(op_stt.first + offset), lfmt);

      return std::make_shared<CombinedFilter>(left, right, op_stt.second);
    }

    // We are now supposed to only have a simple field filter (e.g.: "field_name COMPARATOR value")
    auto [tag_stt_pos, tag_size, comp, is_case_insensitive] = find_next_comparator(fdecl);
    LOG(3, "Found comp %d (case ins: %d) at pos %d, tag is of size %d.\n", comp, is_case_insensitive, tag_stt_pos, tag_size);

    if(tag_stt_pos == std::string::npos){
      throw std::invalid_argument("Could not find any of the recognized comparison operator");
    }

    std::string field_name = fdecl.substr(0, tag_stt_pos); trim(field_name);
    std::string value_str = fdecl.substr(tag_stt_pos + tag_size); trim(value_str);
    LOG(3, "Making filter for field %s with comp %d(ci:%d), value is %s\n", field_name.data(), comp, is_case_insensitive, value_str.data());
    if(field_name == "line_num"){
      if(comp != FilterComparison::CONTAINS){
        throw std::invalid_argument("Special filter linenum must have tag CT or CONTAINS before value");
      }
      // Value str should be "<digit>,<digit>"
      size_t coma_pos = value_str.find(",");
      std::string from_str = value_str.substr(0, coma_pos); trim(from_str);
      std::string to_str = value_str.substr(coma_pos+1); trim(to_str);
      if(!std::isdigit(from_str[0]) || !std::isdigit(to_str[0])){
        throw std::invalid_argument("from/to value of line_num filter is not a valid number");
      }

      line_t from_line = std::stoul(from_str);
      line_t to_line = std::stoul(to_str);

      return std::make_shared<LineNumberFilter>(from_line, to_line);
    }
    return std::make_shared<FieldFilter>(lfmt, field_name, comp, value_str, is_case_insensitive);
  }
}
