#include "line_filter.hpp"
#include "line_format.hpp"
#include "parsing_data.hpp"
#include "processed_line.hpp"

#include <memory>
#include <stdexcept>




CombinedFilter::CombinedFilter(std::shared_ptr<LineFilter> left, std::shared_ptr<LineFilter> right, BitwiseOp op)
  : left_filter(left), right_filter(right), op(op){}

bool CombinedFilter::_passes(const ProcessedLine* pl){ return _passes(pl->pl.get()); }

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


FieldFilter::FieldFilter(LineFormat* lfmt, std::string field_name, FilterComparison cmp, std::string& str_value){
  comp = cmp;

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
    break;
  
  default:
    throw std::runtime_error("Unexpected field type");
    break;
  }

}

FieldFilter::FieldFilter(LineFormat* lfmt, std::string field_name, FilterComparison cmp, void* base_val){
  comp = cmp;

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
  //std::cout << "Checking STR filter with cmp " << comp << " and base val " << val_str << " vs field val " << field_val << std::endl;
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

bool LineNumberFilter::_passes(const ProcessedLine* pl){ 
  return pl->line_num >= line_from && pl->line_num <= line_to;
}

 bool LineNumberFilter::_passes(const ParsedLine* /* unused*/){
  // TODO maybe split interface instead of having this ugly throw;
  throw std::runtime_error("_passes(ParsedLine) called on LineNumberFilter");
}