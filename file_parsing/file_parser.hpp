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
  line_t line_num;
  // Read only, but we need to be carefull about scoping of original data
  std::string_view raw_line;
  bool well_formated;
  ParsedLine* pl;

  ProcessedLine(line_t line, std::string_view sv, Parser* p);
  ~ProcessedLine();


};

class FileParser {
private:
  std::ifstream is;
  LineFormat* line_format;

  //std::vector<ProcessedLine*> pl_pool;
  
  std::vector<std::streampos> blocks_offsets;

  line_t active_line;

  std::vector<ProcessedLine*>* tmp_line_block;
  std::vector<ProcessedLine*>* line_blocks[3];

  
  // Number of lines per block
  int block_size;
  
  // Currently only supports AND between filters
  std::vector<std::shared_ptr<LineFilter>> filters;
  
  bool filters_enabled;
  
  int curr_main_block_id;
  
  std::vector<ProcessedLine*>* getPrevBlock() { return line_blocks[LP_PREV_BLOCK]; }
  std::vector<ProcessedLine*>* getMainBlock() { return line_blocks[LP_MAIN_BLOCK]; }
  std::vector<ProcessedLine*>* getNextBlock() { return line_blocks[LP_NEXT_BLOCK]; }

  line_t getPrevBlockStartLine(){ return (curr_main_block_id-1) * block_size; }
  line_t getMainBlockStartLine(){ return (curr_main_block_id  ) * block_size; }
  line_t getNextBlockStartLine(){ return (curr_main_block_id+1) * block_size; }

  line_t getPrevBlockEndLine(){ return (curr_main_block_id  ) * block_size - 1; }
  line_t getMainBlockEndLine(){ return (curr_main_block_id+1) * block_size - 1; }
  line_t getNextBlockEndLine(){ return (curr_main_block_id+2) * block_size - 1; }

  void slideBlocksForward(int one_or_two);
  void slideBlocksBackward(int one_or_two);

public:

  FileParser(std::string fname, int bsize = 10000);
  ~FileParser();

  void setLineFormat(LineFormat* lf);

  void addFilter(std::shared_ptr<LineFilter> lf);
  void clearFilters();

  void setFiltersEnabled(bool v) { filters_enabled = v; }
  bool areFiltersEnabled() { return filters_enabled; }

  void setActiveLine(line_t line);

  std::vector<std::string_view> getLines(line_t from, line_t count);

  std::vector<line_t> findOccurences(std::string match, line_t from, bool forward = true);
  
};



#endif