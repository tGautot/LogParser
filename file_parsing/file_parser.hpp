#ifndef FILE_PARSER_HPP
#define FILE_PARSER_HPP


#include "parsing_data.hpp"
#include "line_filter.hpp"
#include "line_parser.hpp"

#include <memory>
#include <string>
#include <stdint.h>
#include <fstream>

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

  std::vector<ProcessedLine*> tmp_line_block;
  std::vector<ProcessedLine*> line_blocks[3];

  std::vector<ProcessedLine*>& getPrevBlock() { return line_blocks[0]; }
  std::vector<ProcessedLine*>& getCurrBlock() { return line_blocks[1]; }
  std::vector<ProcessedLine*>& getNextBlock() { return line_blocks[2]; }

  // Number of lines per block
  int block_size;

  // Currently only supports AND between filters
  std::vector<std::shared_ptr<LineFilter>> filters;

  bool filters_enabled;


  void slideBlocksForward(int one_or_two);
  void slideBlocksBackward(int one_or_two);

public:

  FileParser(std::string fname, int bsize = 10000);

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