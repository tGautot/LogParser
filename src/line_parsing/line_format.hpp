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

  void toString(){
    for(LineField* lf : fields){
      std::string name = lf->name;
      LineChrField* lcf;
      LineStrField* lsf;
      switch (lf->ft)
      {
      case FieldType::INT:
        printf("{INT:%s}\n", name.data());
        break;
      case FieldType::DBL:
        printf("{DBL:%s}\n", name.data());
        break;
      case FieldType::CHR:
        lcf = dynamic_cast<LineChrField*>(lf);
        printf("{CHR:%s,%c,%d}\n", name.data(), lcf->opt->target, lcf->opt->repeat);
        break;
      case FieldType::STR:
        lsf = dynamic_cast<LineStrField*>(lf);
        printf("{STR:%s,%d,%c,%d}\n", name.data(), lsf->opt->stop_type, lsf->opt->delim, lsf->opt->nchar);
        break;
      default:
        // TODO throw error
        break;
      }
    }
  }

  static LineFormat* fromFormatString(std::string fmt_str){
    LineFormat* lf = new LineFormat();
    
    // Fmt string might look something like this
    // {INT:time}-{STR:day} {DBL} [{STR:func}:{INT:linenum}] {STR:freetext}
    
    size_t idx = 0;


    int STATE_FREE = 0;
    int STATE_FIELD = 1;
    int parse_state = STATE_FREE; 

    while(idx < fmt_str.size()){
      char c = fmt_str[idx];
      if(c != '{'){
        lf->addField(new LineChrField("", c, false));
        idx++;
      } else {
        // Only available tags at the moment are INT DBL STR CHR, can differentiate just by first char
        // TODO check string length before fetching the chars
        idx++;
        c = fmt_str[idx];
        std::string field_name = "";
        StrFieldStopType stsp;
        int str_n_char = 0;
        char str_stp_chr = 0;
        char field_chr = 0;
        bool chr_repeat = false;
        
        if(fmt_str[idx+3] == ':'){
          size_t name_begin = idx+4;
          size_t name_end = name_begin;
          while(true){
            char cc = fmt_str[name_end]; 
            if(cc == 0 || cc == ',' || cc == '}') break;
            name_end++;
          }
          field_name = fmt_str.substr(name_begin, name_end-name_begin);

          if(',' == fmt_str[name_end]){
            if(c == 'S'){
              str_n_char = atoi(fmt_str.data() + name_end + 1);
              idx = name_end+1;
              while(fmt_str[idx] >= '0' && fmt_str[idx] <= '9'){
                idx++;
              }
              if(fmt_str[idx] != '}') throw std::exception();
              stsp = NCHAR;
              idx++; 
            } else if(c == 'C'){
              idx = name_end+1;
              field_chr = fmt_str[idx];
              if(fmt_str[idx+1] != ',') throw std::exception();
              idx+=2;
              chr_repeat = fmt_str[idx] != '0';
              if(fmt_str[idx+1] != '}') throw std::exception();
              idx += 2;
            } else {
              // TODO throw error, only STR and CHR have params at the moment
              throw std::exception();
              
            }
          }
          else if('}' == fmt_str[name_end] && c == 'S'){
            stsp = DELIM;
            str_stp_chr = fmt_str[name_end+1]; // Also works if fmt_str[name_end+1] == 0 
            idx = name_end+1;
          }
          else{
            idx = name_end + 1;
          }
        }


        switch (c)
        {
        case 'I':
          lf->addField(new LineIntField(field_name));
          break;
        case 'D':
          lf->addField(new LineDblField(field_name));
          break;
        case 'S':
          lf->addField(new LineStrField(field_name, stsp, str_stp_chr, str_n_char));
          break;
        case 'C':
          lf->addField(new LineChrField(field_name, field_chr, chr_repeat));
          break;
        
        default:
          throw std::exception();
          break;
        }



      }
    }



    return lf;
  }
};

#endif
