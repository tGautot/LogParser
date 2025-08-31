#ifndef FILTERED_FILE_READER_HPP
#define FILTERED_FILE_READER_HPP

#include "common.hpp"
#include "line_format.hpp"
#include "line_parser.hpp"
#include "line_filter.hpp"
#include "processed_line.hpp"

#include <deque>

class FilteredFileReader {
  LineFormat* m_lf;
  LineFilter* m_filter;
public:
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
  

// Main public interface
  FilteredFileReader(std::string& fname, LineFormat* lf);
  FilteredFileReader(std::string& fname, LineFormat* lf, LineFilter* filter);

  void reset(bool checkpoints_also = false);
  void setFormat(LineFormat* format);
  void setFilter(LineFilter* filter);

  size_t getMaxCharsPerLine() { return m_max_chars_per_line; }

  void seekRawLine(line_t line_num);
  // Needs to be given storage to store the line (needs at most m_max_chars_per_line bytes)
  // and a ProcessedLine which the function will populate
  // It will return the number of character read/stored (again, at most m_max_chars_per_line)
  // If the function returns 0, there is no garentee as to where the input stream head might be
  size_t getNextValidLine(char* dest, ProcessedLine& pl, line_t stop_at_line = LINE_T_MAX);
 
 
  struct { 
    // Lowest line for which we know the previous valid are the `valid_lines`
    line_t searched_from = 0;
    // Highest line up until which we have searched for valid lines
    line_t searched_to = 0;
    // All the lines we know are valid above `searched_from` lowest index, are lines with lower line_t count
    // The ProcessedLines stored statically here all contain a string_view which points to dynamically allocated data
    // This data must be freed when the PL is discarded
    // To avoid complex memory management, when one result is genven back to the caller, it is deep copied;
    std::deque<ProcessedLine> valid_lines;
  } m_cached_previous;
  size_t getPreviousValidLine(char* dest, ProcessedLine& pl);
  bool eof(){ return m_is.eof(); }

  void invalidateCachedResults();
};

#endif