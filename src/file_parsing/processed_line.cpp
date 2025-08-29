#include "processed_line.hpp"
#include <cstring>


ProcessedLine::ProcessedLine(line_t line, char* s, size_t n_char, Parser* p, std::streampos strm_pos) {
  set_data(line, s, n_char, p, strm_pos);
}

ProcessedLine::ProcessedLine(const ProcessedLine& tocopy, char* copydest, size_t maxcopysize){
  // Use  default copy constructor to populate all fields shallowly
  *this = ProcessedLine(tocopy);
  size_t copysize = std::min(raw_line.size(), maxcopysize);
  std::memcpy(copydest, raw_line.data(), copysize);
  raw_line = std::string_view(copydest, copysize);
}

void  ProcessedLine::set_data(line_t line, char* s, size_t n_char, Parser* p,std::streampos strm_pos) {
  line_num = line;
  raw_line = std::string_view(s, n_char);
  if(p->format == nullptr){
    pl = nullptr;
    well_formated = false;
  } else {
    pl = std::make_shared<ParsedLine>(p->format);
    well_formated = p->parseLine(raw_line, pl.get());
  }
  stt_pos = strm_pos;
}