#include "line_filter.hpp"


LineFilter::LineFilter(LineFormat* lfmt, std::string field_name, FilterComparison cmp, void* base_val, bool neg_cmp){
  comp = cmp;
  neg = neg_cmp;

  targetField = lfmt->getFieldFromName(field_name);
  if(targetField == nullptr){
    // TODO throw error
  }
  parsedFieldId = 0;
  int i;
  for(i = 0; lfmt->fields[i] != targetField; i++){
    if(lfmt->fields[i]->ft == targetField->ft) parsedFieldId++; 
  }


  switch (targetField->ft)
  {
  case FieldType::INT:
    pass_fn = &LineFilter::int_passes;
    val_int = *((int64_t*) base_val);
    break;
  case FieldType::DBL:
    pass_fn = &LineFilter::dbl_passes;
    val_dbl = *((double*) base_val);
    break;
  case FieldType::CHR:
    pass_fn = &LineFilter::chr_passes;
    val_chr = *((char*) base_val);
    break;
  case FieldType::STR:
    pass_fn = &LineFilter::str_passes;
    val_str = std::string(*((char**) base_val));
    break;
  
  default:
    // TODO throw unsupported field type
    break;
  }

}

bool LineFilter::int_passes(ParsedLine* pl){
  //std::cout << "Checking INT filter with cmp " << comp << " and base val " << val_int << std::endl;
  int64_t field_val = *(pl->getIntField(parsedFieldId));
  switch (comp)
  {
  case FilterComparison::EQUAL:
    return field_val == val_int;
  case FilterComparison::SMALLER:
    return field_val < val_int;
  case FilterComparison::GREATER:
    return field_val > val_int;
  default:
    // TODO throw unsupported operation
    return false;
  }
}

bool LineFilter::dbl_passes(ParsedLine* pl){
  //std::cout << "Checking DBL filter with cmp " << comp << " and base val " << val_dbl << std::endl;
  double field_val = *(pl->getDblField(parsedFieldId));
  switch (comp)
  {
  case FilterComparison::EQUAL:
    return field_val == val_dbl;
  case FilterComparison::SMALLER:
    return field_val < val_dbl;
  case FilterComparison::GREATER:
    return field_val > val_dbl;
  default:
    // TODO throw unsupported operation
    return false;
  }
}

bool LineFilter::chr_passes(ParsedLine* pl){
  //std::cout << "Checking CHR filter with cmp " << comp << " and base val " << val_chr << std::endl;
  char field_val = *(pl->getChrField(parsedFieldId));
  switch (comp)
  {
  case FilterComparison::EQUAL:
    return field_val == val_chr;
  case FilterComparison::SMALLER:
    return field_val < val_chr;
  case FilterComparison::GREATER:
    return field_val > val_chr;
  default:
    // TODO throw unsupported operation
    return false;
  }
}

bool LineFilter::str_passes(ParsedLine* pl){
  std::string field_val(*(pl->getStrField(parsedFieldId)));
  //std::cout << "Checking STR filter with cmp " << comp << " and base val " << val_str << " vs field val " << field_val << std::endl;
  switch (comp)
  {
  case FilterComparison::EQUAL:
    return field_val == val_str;
  case FilterComparison::SMALLER:
    return field_val < val_str;
  case FilterComparison::GREATER:
    return field_val > val_str;
  case FilterComparison::CONTAINS:
    return field_val.find(val_str) != std::string::npos;
  case FilterComparison::BEGINS_WITH:
    return field_val.find(val_str) == 0;
  case FilterComparison::ENDS_WITH:
    {
      int sdelta = field_val.length() - val_str.length();
      return (sdelta < 0) ? false : (field_val.find(val_str, sdelta) == sdelta);
    }
  default:
    // TODO throw unsupported operation
    break;
  }
  return false;
}

bool LineFilter::passes(ParsedLine* pl){
  return (this->*pass_fn)(pl);
}