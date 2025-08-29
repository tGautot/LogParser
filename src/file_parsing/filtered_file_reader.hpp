#ifndef FILTERED_FILE_READER_HPP
#define FILTERED_FILE_READER_HPP

#include "common.hpp"
#include "line_format.hpp"
#include "line_parser.hpp"
#include "line_filter.hpp"
#include "processed_line.hpp"

class FilteredFileReader {
private:
  LineFormat* m_lf;
  LineFilter* m_filter;
  Parser* m_line_parser;
  const size_t m_max_chars_per_line;
  bool m_accept_bad_format = false;

  std::ifstream m_is;
  line_t m_curr_line;
  static const line_t m_checkpoint_dist = 1000;
  std::vector<std::streampos> m_checkpoints;
  
  // Pair of [incl; incl] line numbers which we know are filtered out
  std::vector<std::pair<line_t, line_t>> m_filtered_out_lines;

  void goToCheckpoint(line_t cp_id);
  size_t readRawLine(char* s, uint32_t max_chars);
  void skipNextRawLines(line_t n);
  void incrCurrLine();
  int addFilteredOutGroup(line_t stt, line_t end, uint64_t idx);

public:
  FilteredFileReader(std::string& fname, LineFormat* lf);
  FilteredFileReader(std::string& fname, LineFormat* lf, LineFilter* filter);

  size_t getMaxCharsPerLine() { return m_max_chars_per_line; }

  void seekRawLine(line_t line_num);
  // Needs to be given storage to store the line (needs at most m_max_chars_per_line bytes)
  // and a ProcessedLine which the function will populate
  // It will return the number of character read/stored (again, at most m_max_chars_per_line)
  // If the function returns 0, there is no garentee as to where the input stream head might be
  size_t getNextValidLine(char* dest, ProcessedLine& pl, line_t stop_at_line = LINE_T_MAX);
  size_t getPreviousValidLine(char* dest, ProcessedLine& pl);
  bool eof(){ return m_is.eof(); }
};

#endif