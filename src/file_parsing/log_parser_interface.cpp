#include "log_parser_interface.hpp"
#include "common.hpp"
#include "line_filter.hpp"
#include "line_format.hpp"
#include "logging.hpp"
#include "processed_line.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdint.h>
#include <string_view>


LogParserInterface::LogParserInterface(std::string fname, LineFormat* fmt, std::shared_ptr<LineFilter> fltr,  int bsize) 
  : block_size(bsize), active_line(0), block(bsize){
  LOG_LOGENTRY(5, "LogParserInterface::LogParserInterface");
  ffr = new FilteredFileReader(fname, fmt, fltr, 10);
  ProcessedLine pl;
  raw_line_storage = (char*) malloc(bsize * ffr->m_max_chars_per_line * sizeof(char));
  uint32_t nread, line_local_id = 0;
  while( (nread = ffr->getNextValidLine(raw_line_storage + ffr->m_max_chars_per_line*line_local_id, pl)) != 0){
    block.lines.push_back(pl);
    block.raw_lines.push_back(raw_line_storage + ffr->m_max_chars_per_line*line_local_id);
    LOG_FCT(5, "Read %d bytes, line is %s\n", nread, block.lines.back().raw_line.data());
    if(line_local_id == 0) known_first_line = block.lines[0].line_num;
    line_local_id++;
    if(line_local_id == block_size) break; 
  }
  block.flags = BLKFLG_IS_FIRST;
  block.first_line_local_id = 0;
  block.first_line_glbl_id = block.lines.front().line_num;
  block.last_line_glbl_id = block.lines.back().line_num;
  // Special case where first block is first and also last
  if(line_local_id != block_size) {
    block.flags |= BLKFLG_IS_LAST;
    known_last_line = block.lines.back().line_num;
  }
  
}

LogParserInterface::~LogParserInterface(){
  free(raw_line_storage);
  delete ffr;
}

void LogParserInterface::reset_and_refill_block(line_t around_global_line){
  LOG_ENTRY("LogParserInterface::reset_and_refill_block");
  
  block.lines.clear();
  block.raw_lines.clear();

  bool found_anchor =  false;
  ProcessedLine pl;
  uint32_t nread, line_local_id = 0, lines_left = block_size/2;
  while( lines_left > 0  && 
      (nread = ffr->getNextValidLine(raw_line_storage + ffr->m_max_chars_per_line*(line_local_id%block_size), pl)) != 0){
    block.lines.push_back(pl);
    block.raw_lines.push_back(raw_line_storage + ffr->m_max_chars_per_line*line_local_id);
    LOG_FCT(5, "Read %d bytes, line is %s\n", nread, block.lines.back().raw_line.data());
    if(line_local_id == 0) known_first_line = block.lines[0].line_num;
    line_local_id++;
    found_anchor = pl.line_num > around_global_line;
    if(found_anchor){ lines_left--; }
  }

  if(nread == 0){
    block.flags |= BLKFLG_IS_LAST;
    known_last_line = block.lines.back().line_num;
  }

}

void LogParserInterface::setLineFormat(LineFormat* lf, line_t global_ancore_line){
  ffr->setFormat(lf);
  reset_and_refill_block(global_ancore_line);
}

LineFormat* LogParserInterface::getLineFormat(){
  return ffr->m_lf;
}

void LogParserInterface::setFilter(std::shared_ptr<LineFilter> lf, line_t global_ancore_line){
  ffr->setFilter(lf);
  reset_and_refill_block(global_ancore_line);
}

std::shared_ptr<LineFilter> LogParserInterface::getFilter(){
  return ffr->m_filter;
}

void LogParserInterface::print_lines_in_block(){
  printf("current block: flli: %lu - llli: %lu\n", block.first_line_local_id, block.first_line_local_id + block_size);
  for(size_t i = 0; i < block_size; i++){
    printf("\t%s\n", block.lines[i].raw_line.data());
  }
}


line_info_t LogParserInterface::getLine(line_t local_line_id){
  LOG_LOGENTRY(9, "LogParserInterface::getLine");
  LOG_FCT(9, "Looking for line %llu, block rqnge is [%llu, %llu[\n", local_line_id, block.first_line_local_id, block.first_line_local_id + block_size);
  if(local_line_id >= block.first_line_local_id && local_line_id < block.first_line_local_id + block_size){
    if( (block.flags & BLKFLG_IS_LAST) && local_line_id - block.first_line_local_id >= block.size() ){
      return {nullptr, INFO_EOF};
    }
    LOG_FCT(9, "Is in block!\n");
    const ProcessedLine& pl = block.lines[local_line_id - block.first_line_local_id]; 
    line_info_t ret{&pl, 0};
    if(pl.line_num == known_first_line) ret.flags |= INFO_IS_FIRST_LINE;
    if(pl.line_num == known_last_line) ret.flags |= INFO_EOF;
    if(!pl.well_formated) ret.flags |= INFO_IS_MALFORMED;
    return ret;
  }

  // The only way "lines" is not full, is if there are not enough lines in the file to fill it
  // Meaning that the whole file is already in the block, hence we should have found it
  // TODO replace assert with return empty "last line"
  assert(block.lines.full());

  if(local_line_id < block.first_line_local_id){
    LOG_FCT(9, "Is before block!\n");
    if(block.flags & BLKFLG_IS_FIRST) return {nullptr, INFO_ERROR};
    // Requested line is before
    // Fill block by going upwards in the file
    // Since we are using a cyclic deque and it MUST be full, push back will put at front and inversly
    // So lets do a small trick where we put our data at the CURRENT back, and then push front... the back :)
    size_t nread;
    bool got_req_line = false;
    int block_offset = block_size/10;
    int i = block_offset;
    ffr->goToPosition(block.lines.front().stt_pos, block.lines.front().line_num);
    while( (nread = ffr->getPreviousValidLine(block.raw_lines.back(), block.lines.back())) != 0){
      block.raw_lines.push_front(block.raw_lines.back());
      block.lines.push_front(block.lines.back());
      block.first_line_local_id--;
      block.flags &= ~BLKFLG_IS_LAST;
      got_req_line = got_req_line || (block.first_line_local_id == local_line_id);
      if(got_req_line){
        if(i <= 0) break;
        i--;
      }
    }
    bool is_first_block = block.lines[0].line_num == known_first_line;
    if(is_first_block) block.flags |= BLKFLG_IS_FIRST; 
    return getLine(local_line_id);
  }

  // Current case is
  // local_line_id >= block.first_line_local_id + block.size()
  // Requested line is after what block holds
  // Move forward
  if(block.flags & BLKFLG_IS_LAST) return {nullptr, INFO_EOF};
  LOG_FCT(9, "Is after block!\n");
  
  uint32_t nread;
  int block_offset = block_size/10;
  int i = block_offset;
  bool got_req_line =false;
  ffr->goToPosition(block.lines.back().stt_pos, block.lines.back().line_num);
  ffr->skipNextRawLines(1);
  while( (nread = ffr->getNextValidLine(block.raw_lines.front(), block.lines.front())) != 0){
    block.raw_lines.push_back(block.raw_lines.front());
    block.lines.push_back(block.lines.front());
    block.first_line_local_id++;
    block.flags &= ~BLKFLG_IS_FIRST;
    got_req_line = got_req_line || (block.first_line_local_id + block_size - 1 == local_line_id);
    if(got_req_line){
      if(i <= 0) break;
      i--;
    }
  }
  if(nread == 0) {
    known_last_line = block.lines.back().line_num;
    block.flags |= BLKFLG_IS_LAST;
  }
  return getLine(local_line_id);
}

/*
void LogParserInterface::setActiveLine(line_t l){

}

std::vector<std::string_view> LogParserInterface::getLines(line_t from, line_t count){

}
*/

std::pair<line_t, size_t> LogParserInterface::findNextOccurence(std::string match, line_t from, bool forward){
  ffr->goToLine(from);
  char* tmp = new char[ffr->getMaxCharsPerLine()];
  ProcessedLine pl;
  while((forward) ? ffr->getNextValidLine(tmp, pl) != 0 :
                    ffr->getPreviousValidLine(tmp, pl) != 0){
  
    size_t sttpos = pl.raw_line.find(match);
    if(sttpos != std::string::npos){
      // Found match
      delete[] tmp;
      return {pl.line_num, sttpos};
    }
  }
  return {LINE_T_MAX, SIZE_MAX};
}