#ifndef LINE_FORMAT_HPP
#define LINE_FORMAT_HPP

#include <string>
#include <vector>
#include <map>

enum FieldType {
  INT, DBL, CHR, STR
};

class LineField{
public: 
  std::string name;
  FieldType ft;

  LineField(std::string field_name, FieldType field_type) : name(field_name), ft(field_type) {} 
  virtual ~LineField() {}
};



class LineIntField : public LineField {
public:
  LineIntField(std::string field_name) : LineField(field_name, FieldType::INT) {}
  ~LineIntField() {}
};

class LineDblField : public LineField {
public:
  LineDblField(std::string field_name) : LineField(field_name, FieldType::DBL) {}
  ~LineDblField() {}
};

class _ChrFieldOption {
public:
  char target;
  bool repeat;
  _ChrFieldOption(char to_parse, bool do_repeat) : target(to_parse), repeat(do_repeat){}
};

class LineChrField : public LineField {
public:
  _ChrFieldOption* opt;

  LineChrField(std::string field_name, char target, bool repeat) : LineField(field_name, FieldType::CHR) {
    opt = new _ChrFieldOption(target, repeat);
  }
  ~LineChrField() {
    delete opt;
  }
};

enum StrFieldStopType {
  NCHAR, DELIM
};

class _StrFieldOption {
public:
  StrFieldStopType stop_type;
  char delim;
  int nchar;

  _StrFieldOption(StrFieldStopType st, char lim, int n) : stop_type(st), delim(lim), nchar(n){};
};

class LineStrField : public LineField {
public:
  _StrFieldOption* opt;

  LineStrField(std::string field_name, StrFieldStopType st, char lim, int n) : LineField(field_name, FieldType::STR) {
    opt = new _StrFieldOption(st, lim, n);
  }
  ~LineStrField() {
    delete opt;
  }
};



class LineFormat {
  std::map<std::string, LineField*> name_to_field;
public:
  std::vector<LineField*> fields;
  int nint, ndbl, nchr, nstr;

  LineFormat() : nint(0), ndbl(0), nchr(0), nstr(0){}

  int getNIntFields() const { return nint; };
  int getNDoubleFields() const { return ndbl; };
  int getNCharFields() const { return nchr; };
  int getNStringFields() const { return nstr; };

  LineField* getFieldFromName(std::string field_name) {
    if(name_to_field.count(field_name) > 0){
        return name_to_field[field_name];
      }
      return nullptr;
  }

  void addField(LineField* lf) {
    fields.push_back(lf);
    if(lf->name != ""){
      if(name_to_field.count(lf->name) > 0){
        // TODO throw error, cant have same name twice
      }
      name_to_field[lf->name] = lf;
    }
    switch (lf->ft)
    {
    case FieldType::INT:
      nint++;
      break;
    case FieldType::DBL:
      ndbl++;
      break;
    case FieldType::CHR:
      nchr++;
      break;
    case FieldType::STR:
      nstr++;
      break;
    default:
      // TODO throw error
      break;
    }
  }

  static LineFormat* fromFormatString(std::string fmt_str){
    LineFormat* lf = new LineFormat();
    // TODO
    return lf;
  }
};

#endif