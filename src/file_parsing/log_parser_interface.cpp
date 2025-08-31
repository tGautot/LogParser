#include "log_parser_interface.hpp"
#include "line_format.hpp"
#include "parsing_data.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <exception>
#include <deque>
#include <iosfwd>
#include <memory>
#include <string_view>


LogParserInterface::LogParserInterface(std::string fname, LineFormat* fmt, LineFilter* fltr,  int bsize) 
  : block_size(bsize), active_line(0){

  ffr = new FilteredFileReader(fname, fmt, fltr);

  curr_main_block_id = 0;

  line_blocks[LP_PREV_BLOCK] = new std::vector<ProcessedLine*>();
  line_blocks[LP_MAIN_BLOCK] = new std::vector<ProcessedLine*>();
  line_blocks[LP_NEXT_BLOCK] = new std::vector<ProcessedLine*>();
  tmp_line_block = new std::vector<ProcessedLine*>();


  line_blocks[LP_PREV_BLOCK]->resize(block_size);
  line_blocks[LP_MAIN_BLOCK]->resize(block_size);
  line_blocks[LP_NEXT_BLOCK]->resize(block_size);
  tmp_line_block->resize(block_size);

  raw_blocks[0].resize(block_size * 201);
  raw_blocks[1].resize(block_size * 201);
  raw_blocks[2].resize(block_size * 201);

}

LogParserInterface::~LogParserInterface(){
  delete line_blocks[LP_PREV_BLOCK];
  delete line_blocks[LP_MAIN_BLOCK];
  delete line_blocks[LP_NEXT_BLOCK];
  delete tmp_line_block;
}

void LogParserInterface::setLineFormat(LineFormat* lf){
  ffr->setFormat(lf);
}

void LogParserInterface::setFilter(LineFilter* lf){
  ffr->setFilter(lf);
}

void LogParserInterface::clearFilter(){
  // TODO
}


void LogParserInterface::slideBlocksForward(int one_or_two){
  if(one_or_two == 1){
    tmp_line_block = line_blocks[LP_NEXT_BLOCK];
    line_blocks[LP_NEXT_BLOCK] = line_blocks[LP_MAIN_BLOCK];
    line_blocks[LP_MAIN_BLOCK] = line_blocks[LP_PREV_BLOCK];
    line_blocks[LP_PREV_BLOCK] = tmp_line_block;
  } else if(one_or_two == 2){
    slideBlocksForward(1);
    slideBlocksForward(1);
  }
}

void LogParserInterface::slideBlocksBackward(int one_or_two){
if(one_or_two == 1){
    tmp_line_block = line_blocks[LP_PREV_BLOCK];
    line_blocks[LP_PREV_BLOCK] = line_blocks[LP_MAIN_BLOCK];
    line_blocks[LP_MAIN_BLOCK] = line_blocks[LP_NEXT_BLOCK];
    line_blocks[LP_NEXT_BLOCK] = tmp_line_block;
  } else if(one_or_two == 2){
    slideBlocksBackward(1);
    slideBlocksBackward(1);
  }
}

void LogParserInterface::fillBlock(int which){
  
      

}

void LogParserInterface::setActiveLine(line_t l){
  
}

void LogParserInterface::deltaActiveLine(int64_t delta){
  if(delta == 0) return;
  if(delta < 0){

  } else {
    
  }
}

std::vector<std::string_view> LogParserInterface::getLines(line_t from, line_t count){

}

line_t LogParserInterface::findNextOccurence(std::string match, line_t from, bool forward){

}
