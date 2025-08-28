#ifndef FILE_PARSER_HPP
#define FILE_PARSER_HPP


#include "parsing_data.hpp"
#include "line_filter.hpp"
#include "line_parser.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <stdint.h>
#include <fstream>

#define LP_PREV_BLOCK 0
#define LP_MAIN_BLOCK 1
#define LP_NEXT_BLOCK 2

typedef uint64_t line_t;

class ProcessedLine {
public:
  line_t line_num;
  // Read only, but we need to be carefull about scoping of original data
  std::string_view raw_line;
  bool well_formated;
  ParsedLine* pl;

  ProcessedLine(line_t line, char* s, size_t n_char, Parser* p);
  ~ProcessedLine();

  void set_data(line_t line, char* s, size_t n_char, Parser* p);

};

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

  size_t readRawLine(char* s, uint32_t max_chars);
  void skipNextRawLines(line_t n);
  void incrCurrLine();
  void addFilteredOutGroup(line_t stt, line_t end, uint64_t idx);

public:
  FilteredFileReader(std::string& fname, LineFormat* lf);
  FilteredFileReader(std::string& fname, LineFormat* lf, LineFilter* filter);

  size_t getMaxCharsPerLine() { return m_max_chars_per_line; }

  void seekRawLine(line_t line_num);
  // Needs to be given storage to store the line (needs at most m_max_chars_per_line bytes)
  // and a ProcessedLine which the function will populate
  // It will return the number of character read/stored (again, at most m_max_chars_per_line)
  size_t getNextValidLine(char* dest, ProcessedLine& pl);
};

class LogParserInterface {
private:
  FilteredFileReader* ffr;

  //std::vector<ProcessedLine*> pl_pool;
  

  line_t active_line;

  std::vector<ProcessedLine*>* tmp_line_block;
  std::vector<ProcessedLine*>* line_blocks[3];

  std::vector<char> raw_blocks[3];

  
  // Number of lines per block
  int block_size;
  
  // Currently only supports AND between filters
  std::vector<std::shared_ptr<LineFilter>> filters;
  
  bool filters_enabled;
  
  int curr_main_block_id;
  
  std::vector<ProcessedLine*>* getPrevBlock() { return line_blocks[LP_PREV_BLOCK]; }
  std::vector<ProcessedLine*>* getMainBlock() { return line_blocks[LP_MAIN_BLOCK]; }
  std::vector<ProcessedLine*>* getNextBlock() { return line_blocks[LP_NEXT_BLOCK]; }

  void slideBlocksForward(int one_or_two);
  void slideBlocksBackward(int one_or_two);

  void fillBlock(int which);
  void fillPrevBlock(){ fillBlock(LP_PREV_BLOCK); }
  void fillMainBlock(){ fillBlock(LP_MAIN_BLOCK); }
  void fillNextBlock(){ fillBlock(LP_NEXT_BLOCK); }

public:

  LogParserInterface(std::string fname, LineFormat* fmt, LineFilter* fltr, int bsize = 10000);
  ~LogParserInterface();

  void setLineFormat(LineFormat* lf);
  void setFilter(std::shared_ptr<LineFilter> lf);
  void clearFilter();

  void setActiveLine(line_t line);

  std::vector<std::string_view> getFromFirstLine(size_t count);
  std::vector<std::string_view> getLines(line_t from, line_t count);
  std::vector<std::string_view> getFromLastLine(size_t count);

  line_t findNextOccurence(std::string match, line_t from, bool forward = true);
  
};



#endif