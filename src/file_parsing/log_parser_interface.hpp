#ifndef FILE_PARSER_HPP
#define FILE_PARSER_HPP


#include "common.hpp"
#include "processed_line.hpp"
#include "parsing_data.hpp"
#include "line_filter.hpp"
#include "line_parser.hpp"
#include "filtered_file_reader.hpp"
#include <climits>
#include <cstdint>


#define LP_PREV_BLOCK 0
#define LP_MAIN_BLOCK 1
#define LP_NEXT_BLOCK 2


#define BLKFLG_NONE 0
#define BLKFLG_IS_FIRST 1
#define BLKFLG_IS_LAST 2


struct LineBlock {
  std::vector<char> raw_lines;
  std::vector<ProcessedLine> lines;
  // Line nu,ber are global (i.e. number in the file)
  line_t first_line_glbl_id, last_line_glbl_id;
  
  // Local line number, i.e. nth filtered line
  line_t first_line_local_id;
  uint8_t flags;

  LineBlock() = default;
  LineBlock(LineBlock&& tomove){
    flags = tomove.flags;
    first_line_glbl_id = tomove.first_line_glbl_id;
    last_line_glbl_id = tomove.last_line_glbl_id;
    first_line_local_id = tomove.first_line_local_id;
    raw_lines = std::move(tomove.raw_lines);
    lines = std::move(tomove.lines);
  }

  size_t size(){
    return lines.size();
  }
};

#define INFO_NONE 0
#define INFO_EOF 1
#define INFO_IS_FIRST_LINE 2
#define INFO_IS_MALFORMED 4
#define INFO_ERROR 128
typedef struct {
  std::string_view line;
  uint8_t flags;
} line_info_t;


class LogParserInterface {
private:
  FilteredFileReader* ffr;
  line_t active_line=0, known_first_line=0, known_last_line=LINE_MAX;
  uint32_t block_size;
  LineBlock block;


public:

  LogParserInterface(std::string fname, LineFormat* fmt, LineFilter* fltr, int bsize = 10000);
  ~LogParserInterface();

  void setLineFormat(LineFormat* lf);
  void setFilter(LineFilter* lf);
  void clearFilter();

  void setActiveLine(line_t line);
  void deltaActiveLine(int64_t delta);

  //std::vector<std::string_view> getFromFirstLine(size_t count);
  line_info_t getLine(line_t local_line_id);
  //std::vector<std::string_view> getLines(line_t from, line_t count);
  //std::vector<std::string_view> getFromLastLine(size_t count);

  line_t findNextOccurence(std::string match, line_t from, bool forward = true);
  
};



#endif