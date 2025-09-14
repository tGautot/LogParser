#include "log_parser_interface.hpp"
#include "line_format.hpp"
#include "logging.hpp"
#include "processed_line.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <string_view>


LogParserInterface::LogParserInterface(std::string fname, LineFormat* fmt, LineFilter* fltr,  int bsize) 
  : block_size(bsize), active_line(0),block(bsize){
  LOG_LOGENTRY(5, "LogParserInterface::LogParserInterface");
  ffr = new FilteredFileReader(fname, fmt, fltr, 10);
  ProcessedLine pl;
  raw_line_storage = (char*) malloc(bsize * ffr->m_max_chars_per_line * sizeof(char));
  uint32_t raw_data_offset = 0, nread, block_id = 0;
  while( (nread = ffr->getNextValidLine(raw_line_storage + ffr->m_max_chars_per_line*block_id, pl)) != 0){
    block.lines.push_back(pl);
    block.raw_lines.push_back(raw_line_storage + ffr->m_max_chars_per_line*block_id);
    LOG_FCT(5, "Read %d bytes, line is %s\n", nread, block.lines.back().raw_line.data());
    if(block_id == 0) known_first_line = block.lines[0].line_num;
    block_id++;
    if(block_id == block_size) break; 
    raw_data_offset += nread;
  }
  block.lines.resize(block_id);
  block.flags = BLKFLG_IS_FIRST;
  block.first_line_local_id = 0;
  block.first_line_glbl_id = block.lines.front().line_num;
  block.last_line_glbl_id = block.lines.back().line_num;
  // Special case where first block is first and also last
  if(block_id != block_size) {
    block.flags |= BLKFLG_IS_LAST;
    known_last_line = block.lines.back().line_num;
  }
  
}

LogParserInterface::~LogParserInterface(){
  free(raw_line_storage);
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

void LogParserInterface::print_lines_in_block(){
  printf("current block: flli: %lu - llli: %lu\n", block.first_line_local_id, block.first_line_local_id + block_size);
  for(int i = 0; i < block_size; i++){
    printf("\t%s\n", block.lines[i].raw_line.data());
  }
}


line_info_t LogParserInterface::getLine(line_t local_line_id){
  LOG_LOGENTRY(3, "LogParserInterface::getLine");
  LOG_FCT(3, "Looking for line %llu, block rqnge is [%llu, %llu[\n", local_line_id, block.first_line_local_id, block.first_line_local_id + block_size);
  if(local_line_id >= block.first_line_local_id && local_line_id < block.first_line_local_id + block_size){
    LOG_FCT(3, "Is in block!\n");
    const ProcessedLine& pl = block.lines[local_line_id - block.first_line_local_id]; 
    line_info_t ret{pl.raw_line, 0};
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
    LOG_FCT(3, "Is before block!\n");
    if(block.flags & BLKFLG_IS_FIRST) return {"--LOG PARSER ERROR--", INFO_ERROR};
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
    if(is_first_block) block.flags = BLKFLG_IS_FIRST; 
    return getLine(local_line_id);
  }

  // Current case is
  // local_line_id >= block.first_line_local_id + block.size()
  // Requested line is after what block holds
  // Move forward
  // TODO find way to let user know that we have reached eof and line id is invalid
  if(block.flags & BLKFLG_IS_LAST) return {"--LOG PARSER EOF--", INFO_EOF};
  LOG_FCT(3, "Is after block!\n");
  if(block.size() < block_size) block.lines.resize(block_size);
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
  if(i != 0 || block.lines.back().line_num == known_last_line) {
    known_last_line = block.lines.back().line_num;
    block.flags = BLKFLG_IS_LAST;
  }
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