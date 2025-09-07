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
  : block_size(block_size), active_line(0){
  ffr = new FilteredFileReader(fname, fmt, fltr);
  block.lines.reserve(bsize);
  block.raw_lines.reserve(bsize * ffr->m_max_chars_per_line);
  uint32_t raw_data_offset = 0, nread, block_id = 0;
  while( (nread = ffr->getNextValidLine(block.raw_lines.data() + raw_data_offset, block.lines[block_id])) != 0){
    if(block_id == 0) known_first_line = block.lines[0].line_num;
    block_id++;
    if(block_id == block_size) break; 
    raw_data_offset += nread;
  }
  block.lines.resize(block_id);
  block.flags = BLKFLG_IS_FIRST;
  // Special case where first block is first and also last
  if(block_id != block_size) block.flags |= BLKFLG_IS_LAST;
  
}

LogParserInterface::~LogParserInterface(){
}

void LogParserInterface::setLineFormat(LineFormat* lf){
  ffr->setFormat(lf);
}

void LogParserInterface::setFilter(LineFilter* lf){
  ffr->setFilter(lf);
}

void LogParserInterface::clearFilter(){
  ffr->setFilter(nullptr);
}


std::string_view LogParserInterface::getLine(line_t local_line_id){
  if(local_line_id >= block.first_line_local_id && local_line_id < block.first_line_local_id + block.lines.size()){
    return block.lines[block.first_line_local_id - local_line_id].raw_line;
  }

  if(local_line_id < block.first_line_local_id){
    // TODO return info that line doesnt exist (shouldnt happen!!!!)
    if(block.flags | BLKFLG_IS_FIRST) return "";
    // Requested line is before
    // Fill block by going upwards in the file
    uint32_t raw_data_offset = 0;
    if(block.size() < block_size) block.lines.resize(block_size);
    uint32_t block_id = block.size() - 1;
    uint32_t nread;
    while( (nread = ffr->getPreviousValidLine(block.raw_lines.data() + raw_data_offset, block.lines[block_id])) != 0){
      if(block_id == 0) break; 
      block_id--;
      raw_data_offset += nread;
    }
    if(block_id != 0){
      for(int i = block_id; i < block.size(); i++){
        block.lines[i-block_id] = block.lines[i];
      }
      block.lines.resize(block.size() - block_id);
    }
    bool is_first_block = block_id != 0 || block.lines[0].line_num == known_first_line;
    if(is_first_block) block.flags = BLKFLG_IS_FIRST; 
    return getLine(local_line_id);
  }

  // Current case is
  // local_line_id >= block.first_line_local_id + block.size()
  // Requested line is after what block holds
  // Move forward
  // TODO find way to let user know that we have reached eof and line id is invalid
  if(block.flags | BLKFLG_IS_LAST) return "";
  if(block.size() < block_size) block.lines.resize(block_size);
  uint32_t raw_data_offset = 0, nread, block_id = 0;
  while( (nread = ffr->getNextValidLine(block.raw_lines.data() + raw_data_offset, block.lines[block_id])) != 0){
    block_id++;
    if(block_id == block_size) break; 
    raw_data_offset += nread;
  }
  block.lines.resize(block_id);
  if(block_id != block_size) block.flags = BLKFLG_IS_LAST;
  return getLine(local_line_id);
}

/*
void LogParserInterface::setActiveLine(line_t l){

}

std::vector<std::string_view> LogParserInterface::getLines(line_t from, line_t count){

}
*/

line_t LogParserInterface::findNextOccurence(std::string match, line_t from, bool forward){

}