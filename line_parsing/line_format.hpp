#ifndef LINE_FORMAT_HPP
#define LINE_FORMAT_HPP

#include <string>
#include <vector>

enum FieldType {
  INT, DBL, CHR, STR
};

class LineField{
public: 
  std::string name;
  FieldType ft;

  LineField(std::string field_name, FieldType field_type) : name(field_name), ft(field_type) {} 
};



class LineIntField : public LineField {
public:
  LineIntField(std::string field_name) : LineField(field_name, FieldType::INT) {}
};

class LineDblField : public LineField {
public:
  LineDblField(std::string field_name) : LineField(field_name, FieldType::DBL) {}
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
  ~LineChrField(){
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

  /*static _StrFieldOption* untilChar(char delim){
    return new _StrFieldOption(StrFieldStopType::DELIM, delim, 0);
  }
  static _StrFieldOption* forNChar(int n){
    return new _StrFieldOption(StrFieldStopType::NCHAR, 0, n);
  }*/
  

};

class LineStrField : public LineField {
public:
  _StrFieldOption* opt;

  LineStrField(std::string field_name, StrFieldStopType st, char lim, int n) : LineField(field_name, FieldType::STR) {
    opt = new _StrFieldOption(st, lim, n);
  }
};


class LineFormat {
public:
  std::vector<LineField*> fields;
  int nint, ndbl, nchr, nstr;

  int getNIntFields() const { return nint; };
  int getNDoubleFields() const { return ndbl; };
  int getNCharFields() const { return nchr; };
  int getNStringFields() const { return nstr; };

  void addField(LineField* lf) {
    fields.push_back(lf);
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