#include "line_filter.hpp"
#include "line_format.hpp"
#include "parsing_data.hpp"
#include "processed_line.hpp"
#include "string_utils.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>





CombinedFilter::CombinedFilter(std::shared_ptr<LineFilter> left, std::shared_ptr<LineFilter> right, BitwiseOp op)
  : left_filter(left), right_filter(right), op(op){}


// A complete copy paste between the two funcs of CombinedFilter....

bool CombinedFilter::_passes(const ProcessedLine* pl){ 
    bool left_ok = left_filter->passes(pl);

    if( left_ok && op==OR ) return true;
    if(!left_ok && op==AND) return false;
    if( left_ok && op==NOR) return false;

    bool right_ok = right_filter->passes(pl);

    switch (op) {
      case AND:
        return  left_ok && right_ok;
      case OR:
        return  left_ok || right_ok;
      case XOR:
        return  left_ok ^  right_ok;
      case NOR:
        return !(left_ok || right_ok);
      default:
        throw std::runtime_error("Forgot to handle op of combined filter");
    }

}

bool CombinedFilter::_passes(const ParsedLine* pl) {
  bool left_ok = left_filter->passes(pl);

  if( left_ok && op==OR ) return true;
  if(!left_ok && op==AND) return false;
  if( left_ok && op==NOR) return false;

  bool right_ok = right_filter->passes(pl);

  switch (op) {
    case AND:
      return  left_ok && right_ok;
    case OR:
      return  left_ok || right_ok;
    case XOR:
      return  left_ok ^  right_ok;
    case NOR:
      return !(left_ok || right_ok);
    default:
      throw std::runtime_error("Forgot to handle op of combined filter");
  }


}


FieldFilter::FieldFilter(LineFormat* lfmt, std::string field_name, FilterComparison cmp, std::string& str_value, bool str_case_ins_check){
  comp = cmp;
  case_insensitive_check = str_case_ins_check;

  targetField = lfmt->getFieldFromName(field_name);
  if(targetField == nullptr){
    throw std::invalid_argument("FieldFilter error: Couldn't find field named " + field_name);
  }
  parsedFieldId = 0;
  int i;
  for(i = 0; lfmt->fields[i] != targetField; i++){
    if(lfmt->fields[i]->ft == targetField->ft) parsedFieldId++; 
  }


  switch (targetField->ft)
  {
  case FieldType::INT:
    pass_fn = &FieldFilter::int_passes;
    val_int = std::stol(str_value);
    break;
  case FieldType::DBL:
    pass_fn = &FieldFilter::dbl_passes;
    val_dbl = std::stod(str_value);
    break;
  case FieldType::CHR:
    pass_fn = &FieldFilter::chr_passes;
    val_chr = str_value[0];
    break;
  case FieldType::STR:
    pass_fn = &FieldFilter::str_passes;
    val_str = str_value;
    if(case_insensitive_check){
      std::transform(val_str.begin(), val_str.end(), val_str.begin(), ::tolower);
    }
    break;
   
  default:
    throw std::runtime_error("Unexpected field type");
    break;
  }

}

FieldFilter::FieldFilter(LineFormat* lfmt, std::string field_name, FilterComparison cmp, void* base_val, bool str_case_ins_check){
  comp = cmp;
  case_insensitive_check = str_case_ins_check;

  targetField = lfmt->getFieldFromName(field_name);
  if(targetField == nullptr){
    throw std::invalid_argument("FieldFilter error: Couldn't find field named " + field_name);
  }
  parsedFieldId = 0;
  int i;
  for(i = 0; lfmt->fields[i] != targetField; i++){
    if(lfmt->fields[i]->ft == targetField->ft) parsedFieldId++; 
  }


  switch (targetField->ft)
  {
  case FieldType::INT:
    pass_fn = &FieldFilter::int_passes;
    val_int = *((int64_t*) base_val);
    break;
  case FieldType::DBL:
    pass_fn = &FieldFilter::dbl_passes;
    val_dbl = *((double*) base_val);
    break;
  case FieldType::CHR:
    pass_fn = &FieldFilter::chr_passes;
    val_chr = *((char*) base_val);
    break;
  case FieldType::STR:
    pass_fn = &FieldFilter::str_passes;
    val_str = std::string(*((char**) base_val));
    if(case_insensitive_check){
      std::transform(val_str.begin(), val_str.end(), val_str.begin(), ::tolower);
    }
    break;
  
  default:
    throw std::runtime_error("Unexpected field type");
    break;
  }

}

bool FieldFilter::int_passes(const ParsedLine* pl){
  //std::cout << "Checking INT filter with cmp " << comp << " and base val " << val_int << std::endl;
  int64_t field_val = *(pl->getIntField(parsedFieldId));
  switch (comp)
  {
  case FilterComparison::EQUAL:
    return field_val == val_int;
  case FilterComparison::SMALLER:
    return field_val < val_int;
  case FilterComparison::SMALLER_EQ:
    return field_val <= val_int;
  case FilterComparison::GREATER:
    return field_val > val_int;
  case FilterComparison::GREATER_EQ:
    return field_val >= val_int;
  default:
    throw std::logic_error("Filter on field " + targetField->name + " has unsupported operation for type INT");
    return false;
  }
}

bool FieldFilter::dbl_passes(const ParsedLine* pl){
  //std::cout << "Checking DBL filter with cmp " << comp << " and base val " << val_dbl << std::endl;
  double field_val = *(pl->getDblField(parsedFieldId));
  switch (comp)
  {
  case FilterComparison::EQUAL:
    return field_val == val_dbl;
  case FilterComparison::SMALLER:
    return field_val < val_dbl;
  case FilterComparison::SMALLER_EQ:
    return field_val <= val_dbl;
  case FilterComparison::GREATER:
    return field_val > val_dbl;
  case FilterComparison::GREATER_EQ:
    return field_val >= val_dbl;
  default:
    throw std::logic_error("Filter on field " + targetField->name + " has unsupported operation for type DBL");
    return false;
  }
}

bool FieldFilter::chr_passes(const ParsedLine* pl){
  //std::cout << "Checking CHR filter with cmp " << comp << " and base val " << val_chr << std::endl;
  char field_val = *(pl->getChrField(parsedFieldId));
  switch (comp)
  {
  case FilterComparison::EQUAL:
    return field_val == val_chr;
  case FilterComparison::SMALLER:
    return field_val < val_chr;
  case FilterComparison::SMALLER_EQ:
    return field_val <= val_chr;
  case FilterComparison::GREATER:
    return field_val > val_chr;
  case FilterComparison::GREATER_EQ:
    return field_val >= val_chr;
  default:
    throw std::logic_error("Filter on field " + targetField->name + " has unsupported operation for type CHR");
    return false;
  }
}

bool FieldFilter::str_passes(const ParsedLine* pl){
  std::string_view field_val(*(pl->getStrField(parsedFieldId)));
  std::string lowered;
  if(case_insensitive_check){
    // Make a copy of the field value, lower it, and recreate field_val from it
    lowered = std::string(field_val.data(), field_val.size());
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), ::tolower);
    field_val = lowered;
  }
  
  switch (comp)
  {
  case FilterComparison::EQUAL:
    return field_val == val_str;
  case FilterComparison::SMALLER:
    return field_val < val_str;
  case FilterComparison::SMALLER_EQ:
    return field_val <= val_str;
  case FilterComparison::GREATER:
    return field_val > val_str;
  case FilterComparison::GREATER_EQ:
    return field_val >= val_str;
  case FilterComparison::CONTAINS:
    return field_val.find(val_str) != std::string::npos;
  case FilterComparison::BEGINS_WITH:
    return field_val.find(val_str) == 0;
  case FilterComparison::ENDS_WITH:
    {
      int sdelta = field_val.length() - val_str.length();
      return (sdelta < 0) ? false : (field_val.find(val_str, sdelta) == (size_t) sdelta);
    }
  default:
    throw std::logic_error("Filter on field " + targetField->name + " has unsupported operation for type STR");
    break;
  }
  return false;
}

bool FieldFilter::_passes(const ProcessedLine* pl){ return _passes(pl->pl.get()); }

bool FieldFilter::_passes(const ParsedLine* pl){
  return (this->*pass_fn)(pl);
}

LineNumberFilter::LineNumberFilter(line_t from, line_t to) :  line_from(from), line_to(to) {}

bool LineNumberFilter::_passes(const ProcessedLine* pl){ 
  return pl->line_num >= line_from && pl->line_num <= line_to;
}

 bool LineNumberFilter::_passes(const ParsedLine* /* unused*/){
  // TODO maybe split interface instead of having this ugly throw;
  throw std::runtime_error("_passes(ParsedLine) called on LineNumberFilter");
}

RawLineFilter::RawLineFilter(std::string& s) : must_contain(s){}

bool RawLineFilter::_passes(const ProcessedLine* pl){ 
  return pl->raw_line.find(must_contain) != std::string::npos;
}

 bool RawLineFilter::_passes(const ParsedLine* /* unused*/){
  // TODO maybe split interface instead of having this ugly throw;
  throw std::runtime_error("_passes(ParsedLine) called on LineNumberFilter");
}

// ──────────────────────────────────────────────
// to_string / equals
// ──────────────────────────────────────────────

static std::string comparison_to_string(FilterComparison comp){
  switch(comp){
  case EQUAL:       return "EQ";
  case SMALLER:     return "ST";
  case GREATER:     return "GT";
  case GREATER_EQ:  return "GE";
  case SMALLER_EQ:  return "SE";
  case CONTAINS:    return "CT";
  case BEGINS_WITH: return "BW";
  case ENDS_WITH:   return "EW";
  default: throw std::logic_error("Unknown FilterComparison in to_string");
  }
}

static std::string bitwise_op_to_string(BitwiseOp op){
  switch(op){
  case AND: return "AND";
  case OR:  return "OR";
  case XOR: return "XOR";
  case NOR: return "NOR";
  default: throw std::logic_error("Unknown BitwiseOp in to_string");
  }
}

std::string FieldFilter::to_string() const {
  std::string tag = comparison_to_string(comp);
  if(case_insensitive_check) tag += "_CI";

  std::string value;
  switch(targetField->ft){
  case FieldType::INT: value = std::to_string(val_int); break;
  case FieldType::DBL: value = std::to_string(val_dbl); break;
  case FieldType::CHR: value = std::string(1, val_chr); break;
  case FieldType::STR: value = val_str; break;
  default: throw std::logic_error("Unknown field type in to_string");
  }
  return targetField->name + " " + tag + " " + value;
}

bool FieldFilter::equals(const LineFilter& other) const {
  const auto* o = dynamic_cast<const FieldFilter*>(&other);
  if(!o) return false;
  if(targetField->name != o->targetField->name) return false;
  if(targetField->ft != o->targetField->ft) return false;
  if(comp != o->comp) return false;
  if(case_insensitive_check != o->case_insensitive_check) return false;
  switch(targetField->ft){
  case FieldType::INT: return val_int == o->val_int;
  case FieldType::DBL: return val_dbl == o->val_dbl;
  case FieldType::CHR: return val_chr == o->val_chr;
  case FieldType::STR: return val_str == o->val_str;
  default: return false;
  }
}

std::string LineNumberFilter::to_string() const {
  return "line_num CT " + std::to_string(line_from) + "," + std::to_string(line_to);
}

bool LineNumberFilter::equals(const LineFilter& other) const {
  const auto* o = dynamic_cast<const LineNumberFilter*>(&other);
  if(!o) return false;
  return line_from == o->line_from && line_to == o->line_to;
}

std::string CombinedFilter::to_string() const {
  return "(" + left_filter->to_string() + ") " + bitwise_op_to_string(op) + " (" + right_filter->to_string() + ")";
}

bool CombinedFilter::equals(const LineFilter& other) const {
  const auto* o = dynamic_cast<const CombinedFilter*>(&other);
  if(!o) return false;
  return op == o->op && left_filter->equals(*o->left_filter) && right_filter->equals(*o->right_filter);
}

std::string RawLineFilter::to_string() const {
  return "raw_contains:" + must_contain;
}

bool RawLineFilter::equals(const LineFilter& other) const {
  const auto* o = dynamic_cast<const RawLineFilter*>(&other);
  if(!o) return false;
  return must_contain == o->must_contain;
}