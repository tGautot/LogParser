#include "log_parser_interface.hpp"
#include "line_format.hpp"
#include "parsing_data.hpp"
#include "logging.hpp"
#include "processed_line.hpp"

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
  LOG_LOGENTRY(5, "LogParserInterface::LogParserInterface");
  ffr = new FilteredFileReader(fname, fmt, fltr, 10);
  block.lines.resize(bsize);
  block.raw_lines.resize(bsize * ffr->m_max_chars_per_line);
  uint32_t raw_data_offset = 0, nread, block_id = 0;
  while( (nread = ffr->getNextValidLine(block.raw_lines.data() + raw_data_offset, block.lines[block_id])) != 0){
    LOG_FCT(5, "Read %d bytes, line is %s\n", nread, block.lines[block_id].raw_line.data());
    if(block_id == 0) known_first_line = block.lines[0].line_num;
    block_id++;
    if(block_id == block_size) break; 
    raw_data_offset += nread;
  }
  block.lines.resize(block_id);
  block.flags = BLKFLG_IS_FIRST;
  block.first_line_local_id = 0;
  block.first_line_glbl_id = block.lines[0].line_num;
  block.last_line_glbl_id = block.lines.back().line_num;
  // Special case where first block is first and also last
  if(block_id != block_size) block.flags |= BLKFLG_IS_LAST;
  
}

LogParserInterface::~LogParserInterface(){
  delete ffr;
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


line_info_t LogParserInterface::getLine(line_t local_line_id){
  LOG_LOGENTRY(5, "LogParserInterface::getLine");
  LOG_FCT(5, "Looking for line %llu, block rqnge is [%llu, %llu[\n", local_line_id, block.first_line_local_id, block.first_line_local_id + block.lines.size());
  if(local_line_id >= block.first_line_local_id && local_line_id < block.first_line_local_id + block.lines.size()){
    LOG_FCT(5, "Is in block!\n");
    const ProcessedLine& pl = block.lines[local_line_id - block.first_line_local_id]; 
    line_info_t ret{pl.raw_line, 0};
    if(pl.line_num == known_first_line) ret.flags |= INFO_IS_FIRST_LINE;
    if(pl.line_num == known_last_line) ret.flags |= INFO_EOF;
    if(!pl.well_formated) ret.flags |= INFO_IS_MALFORMED;
    return ret;
  }

  if(local_line_id < block.first_line_local_id){
    LOG_FCT(5, "Is before block!\n");
    // TODO return info that line doesnt exist (shouldnt happen!!!!)
    if(block.flags | BLKFLG_IS_FIRST) return {"", INFO_ERROR};
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
  if(block.flags | BLKFLG_IS_LAST) return {"", INFO_EOF};
  LOG_FCT(5, "Is after block!\n");
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