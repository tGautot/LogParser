#ifndef PROCESSED_LINE_HPP
#define PROCESSED_LINE_HPP


#include "common.hpp"
#include "line_parser.hpp"

// Quick to copy
class ProcessedLine {
public:
  std::streampos stt_pos;
  line_t line_num;
  // Read only, but we need to be carefull about scoping of original data
  std::string_view raw_line;
  std::shared_ptr<ParsedLine> pl;
  bool well_formated;

  ProcessedLine() = default;
  ProcessedLine(line_t line, char* s, size_t n_char, Parser* p, std::streampos strm_pos);
  ProcessedLine(const ProcessedLine& tocopy, char* copydest, size_t maxcopysize);
  ~ProcessedLine() = default;

  void set_data(line_t line, char* s, size_t n_char, Parser* p,std::streampos strm_pos);

};


#endif