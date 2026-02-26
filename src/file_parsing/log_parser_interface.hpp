#ifndef FILE_PARSER_HPP
#define FILE_PARSER_HPP


#include "common.hpp"
#include "processed_line.hpp"
#include "line_filter.hpp"
#include "filtered_file_reader.hpp"
#include <climits>
#include <cstdint>
#include <memory>
#include "cyclic_deque.hpp"

#define BLKFLG_NONE 0
#define BLKFLG_IS_FIRST 1
#define BLKFLG_IS_LAST 2


struct LineBlock {
  cyclic_deque<ProcessedLine> lines;
  cyclic_deque<char*> raw_lines;

  
  // Local line number, i.e. nth filtered line
  line_t first_line_local_id;
  uint8_t flags;

  LineBlock(size_t max_size):lines(max_size), raw_lines(max_size){};
  LineBlock(LineBlock&& tomove): lines(std::move(tomove.lines)), raw_lines(std::move(tomove.raw_lines)){
    flags = tomove.flags;
    first_line_local_id = tomove.first_line_local_id;
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
  const ProcessedLine* line;
  uint8_t flags;
} line_info_t;


class LogParserInterface {
private:
  line_t  known_first_line=0;
  char* raw_line_storage;
  FilteredFileReader* ffr;

  void print_lines_in_block();

  void reset_and_refill_block(line_t around_global_line);

  // Records mapping local->global for one in every 100 local line
  std::vector<line_t> local_to_global_id;

public:
  uint32_t block_size;
  LineBlock block;
  line_t known_last_line=LINE_MAX;
  LogParserInterface(std::string fname, LineFormat* fmt, std::shared_ptr<LineFilter> fltr, int bsize = 10000);
  ~LogParserInterface();

  void setLineFormat(LineFormat* lf, line_t global_ancore_line);
  LineFormat* getLineFormat();
  void setFilter(std::shared_ptr<LineFilter> lf, line_t global_ancore_line = 1);
  std::shared_ptr<LineFilter> getFilter();

  void setActiveLine(line_t line);
  void deltaActiveLine(int64_t delta);

  line_t getLineGlobalIdLowerbound(line_t local_line_id);
  line_t getLineGlobalIdUpperbound(line_t local_line_id);

  //std::vector<std::string_view> getFromFirstLine(size_t count)
  

  /**
    * This is the main, simplest function to get a line in LOCAL ID
    * from the file. This function call expects some locality between
    * successive calls, don't use to to jump between very far away places
    * in the file, as this function will read everything that is in between
    * For this use jumpTo(Local/Global)Line instead
    */
  line_info_t getLine(line_t local_line_id);

  void jumpToLocalLine(line_t local_line_id);
  void jumpToGlobalLine(line_t global_line_id);
  //std::vector<std::string_view> getLines(line_t from, line_t count);
  //std::vector<std::string_view> getFromLastLine(size_t count);

  std::pair<line_t, size_t> findNextOccurence(std::string match, line_t from, bool forward = true);
  
};



#endif