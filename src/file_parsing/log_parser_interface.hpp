#ifndef FILE_PARSER_HPP
#define FILE_PARSER_HPP


#include "common.hpp"
#include "processed_line.hpp"
#include "parsing_data.hpp"
#include "line_filter.hpp"
#include "line_parser.hpp"
#include "filtered_file_reader.hpp"


#define LP_PREV_BLOCK 0
#define LP_MAIN_BLOCK 1
#define LP_NEXT_BLOCK 2



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
  void setFilter(LineFilter* lf);
  void clearFilter();

  void setActiveLine(line_t line);
  void deltaActiveLine(int64_t delta);

  std::vector<std::string_view> getFromFirstLine(size_t count);
  std::vector<std::string_view> getLines(line_t from, line_t count);
  std::vector<std::string_view> getFromLastLine(size_t count);

  line_t findNextOccurence(std::string match, line_t from, bool forward = true);
  
};



#endif