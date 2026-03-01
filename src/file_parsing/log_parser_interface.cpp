#include "log_parser_interface.hpp"
#include "common.hpp"
#include "line_filter.hpp"
#include "line_format.hpp"
#include "logging.hpp"
#include "processed_line.hpp"

#include <cassert>
#include <climits>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <string_view>


LogParserInterface::LogParserInterface(std::string fname, LineFormat* fmt, std::shared_ptr<LineFilter> fltr,  int bsize) 
  : block_size(bsize), block(bsize){
  LOG_LOGENTRY(5, "LogParserInterface::LogParserInterface");
  ffr = new FilteredFileReader(fname, fmt, fltr, 10);
  known_last_line = LINE_MAX;
  known_first_line = 0;

  ProcessedLine pl;
  raw_line_storage = (char*) malloc(bsize * ffr->m_max_chars_per_line * sizeof(char));
  uint32_t nread, line_local_id = 0;
  while( line_local_id < block_size && 
      (nread = ffr->getNextValidLine(raw_line_storage + ffr->m_max_chars_per_line*line_local_id, pl)) != 0){
    block.lines.push_back(pl);
    block.raw_lines.push_back(raw_line_storage + ffr->m_max_chars_per_line*line_local_id);
    LOG_FCT(5, "Read %d bytes, line is %s\n", nread, block.lines.back().raw_line.data());
    if(line_local_id % 100 == 0 && line_local_id/100 >= local_to_global_id.size()){
      local_to_global_id.push_back(pl.line_num);
    }
    line_local_id++;
    if(line_local_id == block_size) break; 
  }
  known_first_line = block.lines[0].line_num;
  block.first_line_local_id = 0;
  // Special case where first block is first and also last
  if(nread == 0) {
    block.contains_last_line |= true;
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
  known_last_line = LINE_MAX;
  known_first_line = 0;

  bool found_anchor =  false;
  ProcessedLine pl;
  uint32_t nread, line_local_id = 0, lines_left = block_size/2;
  ffr->goToLine(0);
  block.first_line_local_id = 0;
  while( lines_left > 0  && 
      (nread = ffr->getNextValidLine(raw_line_storage + ffr->m_max_chars_per_line*(line_local_id%block_size), pl)) != 0){
    if(block.lines.full()){
      block.first_line_local_id++;
    }
    block.lines.push_back(pl);
    block.raw_lines.push_back(raw_line_storage + ffr->m_max_chars_per_line*(line_local_id%block_size));
    LOG_FCT(5, "Read %d bytes, line is %s\n", nread, block.lines.back().raw_line.data());
    if(line_local_id == 0) known_first_line = block.lines[0].line_num;
    if(line_local_id % 100 == 0 && line_local_id/100 >= local_to_global_id.size()){
      local_to_global_id.push_back(pl.line_num);
    }
    line_local_id++;
    found_anchor = pl.line_num > around_global_line;
    if(found_anchor){ lines_left--; }
  }

  if(nread == 0){
    block.contains_last_line = true;
    known_last_line = block.lines.back().line_num;
  }

}

void LogParserInterface::setLineFormat(LineFormat* lf, line_t global_ancore_line){
  ffr->setFormat(lf);
  reset_and_refill_block(global_ancore_line);
  local_to_global_id.clear();
}

LineFormat* LogParserInterface::getLineFormat(){
  return ffr->m_lf;
}

void LogParserInterface::setFilter(std::shared_ptr<LineFilter> lf, line_t global_ancore_line){
  ffr->setFilter(lf);
  reset_and_refill_block(global_ancore_line);
  local_to_global_id.clear();
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
  if(local_line_id >= block.first_line_local_id && (local_line_id < block.first_line_local_id + block_size || block.contains_last_line) ){
    if( (block.contains_last_line) && local_line_id - block.first_line_local_id >= block.size() ){
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

    // Should not happen since local_line_id is line_t
    if(block.first_line_local_id == 0) return {nullptr, INFO_ERROR};


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
      block.contains_last_line = false;
      got_req_line = got_req_line || (block.first_line_local_id == local_line_id);
      if(got_req_line){
        if(i <= 0) break;
        i--;
      }
    }
    return getLine(local_line_id);
  }

  // Current case is
  // local_line_id >= block.first_line_local_id + block.size()
  // Requested line is after what block holds
  // Move forward
  if(block.contains_last_line) return {nullptr, INFO_EOF};
  LOG_FCT(9, "Is after block!\n");
  
  uint32_t nread;
  int block_offset = block_size/10;
  int i = block_offset;
  line_t block_last_line_local_id = block.first_line_local_id + block.lines.size() - 1;
  bool got_req_line =false;
  ffr->goToPosition(block.lines.back().stt_pos, block.lines.back().line_num);
  ffr->skipNextRawLines(1);
  while( (nread = ffr->getNextValidLine(block.raw_lines.front(), block.lines.front())) != 0){
    block.raw_lines.push_back(block.raw_lines.front());
    block.lines.push_back(block.lines.front());
    block.first_line_local_id++;
    
    block_last_line_local_id++;
    if(block_last_line_local_id % 100 == 0){
      if(block_last_line_local_id/100 > local_to_global_id.size()){
        throw std::runtime_error("Missed a local<->global mapping");
      }
      if(block_last_line_local_id/100 == local_to_global_id.size())
        local_to_global_id.push_back(block.lines.back().line_num);
    }

    got_req_line = got_req_line || (block.first_line_local_id + block_size - 1 == local_line_id);
    if(got_req_line){
      if(i <= 0) break;
      i--;
    }
  }
  if(nread == 0) {
    known_last_line = block.lines.back().line_num;
    block.contains_last_line = true;
  }
  return getLine(local_line_id);
}

line_t LogParserInterface::getLineGlobalIdLowerbound(line_t local_line_id){
  size_t idx = local_line_id/100;
  if(idx >= local_to_global_id.size()) return local_to_global_id.back();
  return local_to_global_id[idx];
}
line_t LogParserInterface::getLineGlobalIdUpperbound(line_t local_line_id){
  size_t idx = local_line_id/100+1;
  if(idx >= local_to_global_id.size()) return LINE_MAX;
  return local_to_global_id[idx];

}

void LogParserInterface::jumpToLocalLine(line_t local_line_id){
  LOG_LOGENTRY(5, "LogParserInterface::jumpToLocalLine");
  LOG_FCT(5,"Jumping to local line %ld\n", local_line_id);
  line_t block_last_line = block.first_line_local_id + block_size;
  if(local_line_id >= block.first_line_local_id && (local_line_id <= block_last_line || block.contains_last_line)){
    // Already in block
    LOG_EXIT();
    return;
  }
 

  line_t around_global_line; // Global id of the line around which the block might need to be formed
  line_t around_local_line; // local id of "around_line"

  if(local_line_id < block.first_line_local_id){
    LOG_FCT(5, "Local line is above what current block holds\n");
    if(block.first_line_local_id - local_line_id < block_size){
      // Go there step by step, that way the block will be already correctly populated
      getLine(local_line_id);
      LOG_EXIT();
      return;
    }
    // Line is too far before in the file
    // But since it is _before_, we must have gone there already and thus know the lower bound
    around_global_line = getLineGlobalIdLowerbound(local_line_id);
    around_local_line = local_line_id - (local_line_id%100);
    
  } else { // local > last line of block
    LOG_FCT(5, "Local line is later what current block holds\n");
    if(local_line_id - block_last_line < block_size){
      getLine(local_line_id);
      LOG_EXIT();
      return;
    }

    // Line is later in the file
    // Maybe we already saw it, in which case we probably know an upperbound,
    // If not, it means we never saw it, and we need to go there sequentially
    around_global_line = getLineGlobalIdUpperbound(local_line_id);
    around_local_line = local_line_id - (local_line_id%100) + 100;

    if(around_global_line == LINE_MAX){  // Unkown best we can do is go as far as we explored, and walk from there    
      line_t lb;

      assert(local_to_global_id.size() > 0);

      size_t idx = std::min(local_line_id/100, local_to_global_id.size()-1);
      lb = local_to_global_id[idx];
      line_t start_local_line = idx*100;
      
      block.lines.clear();
      block.raw_lines.clear();
      
      bool found_line =  false;
      ProcessedLine pl;
      uint32_t nread, last_read_local_id = start_local_line, lines_left = block_size/2;
      ffr->goToLine(lb);
      block.first_line_local_id = start_local_line;
      LOG_FCT(5, "Requested line is in uncharted teritory, going on an adventure starting at (g:%lu,l:%lu)\n", lb, start_local_line);
      
      while( lines_left > 0  && 
          (nread = ffr->getNextValidLine(raw_line_storage + ffr->m_max_chars_per_line*(last_read_local_id%block_size), pl)) != 0){
        if(block.lines.full()){
          block.first_line_local_id++;
        }
        block.lines.push_back(pl);
        block.raw_lines.push_back(raw_line_storage + ffr->m_max_chars_per_line*(last_read_local_id%block_size));
        if(last_read_local_id == 0) known_first_line = block.lines[0].line_num;
        if(last_read_local_id % 100 == 0 && last_read_local_id/100 >= local_to_global_id.size()){
          local_to_global_id.push_back(pl.line_num);
        }
        last_read_local_id++;
        found_line = pl.line_num > local_line_id;
        if(found_line){ lines_left--; }
      }

      if(nread == 0){
        known_last_line = block.lines.back().line_num;
        block.contains_last_line = true;
      }

      if(!block.lines.full()){ // Wasn't too far from lower bound, and found it without filling whole block
        ffr->goToLine(lb);
        lines_left = block_size-block.lines.size();
        last_read_local_id = start_local_line-1;
        while( lines_left > 0  && 
            (nread = ffr->getPreviousValidLine(raw_line_storage + ffr->m_max_chars_per_line*(last_read_local_id%block_size), pl)) != 0){
          block.lines.push_front(pl);
          block.raw_lines.push_front(raw_line_storage + ffr->m_max_chars_per_line*(last_read_local_id%block_size));
          last_read_local_id--;
          block.first_line_local_id--;
          lines_left--;
        }
      }

      if(nread == 0){
        known_last_line = block.lines.front().line_num;
        block.contains_last_line = true;
      }

      LOG_EXIT();
      return;
    } 
  }

  LOG_FCT(5, "Jumping to a known place of the file, arougn global line %lu\n", around_global_line);
  size_t segment_lines_left = block_size/2, nread;
  ProcessedLine pl;
  size_t line_storage_id = 0;
  bool no_more_above = false, no_more_below = false, finished = false;
  block.lines.clear();
  block.raw_lines.clear();
  ffr->goToLine(around_global_line);
  block.first_line_local_id = around_local_line;
read_backwards:
  LOG_FCT(5, "Reading backwards (%ld lines max)\n", segment_lines_left);
  while( segment_lines_left > 0  && 
      (nread = ffr->getPreviousValidLine(raw_line_storage + ffr->m_max_chars_per_line*line_storage_id, pl)) != 0){
    block.lines.push_front(pl);
    block.raw_lines.push_front(raw_line_storage + ffr->m_max_chars_per_line*line_storage_id);
    line_storage_id++;
    block.first_line_local_id--;
    segment_lines_left--;
  }
  no_more_above = nread == 0;
  segment_lines_left = block_size - line_storage_id;
  finished = (line_storage_id == block_size) || no_more_below;
  if(finished) goto done;
  ffr->goToLine(around_global_line);
read_forward:
  LOG_FCT(5, "Reading forward (%ld lines max)\n", segment_lines_left);
  while( segment_lines_left > 0  && 
      (nread = ffr->getNextValidLine(raw_line_storage + ffr->m_max_chars_per_line*line_storage_id, pl)) != 0){
    block.lines.push_back(pl);
    block.raw_lines.push_back(raw_line_storage + ffr->m_max_chars_per_line*line_storage_id);
    line_storage_id++;
    segment_lines_left--;
  }
  no_more_below = nread == 0;
  segment_lines_left = block_size - line_storage_id;
  finished = (line_storage_id == block_size) || (no_more_above && no_more_below);
  if(finished) goto done;

  // Not finished, either
  // Could read half block above, but not below (try again, above)
  // OR
  // Could read half block below, but not above (try again, below)
  // OR
  // Could not read half block in either direction (doomed, should never happen as file fits into 1 block)

  if(no_more_below && !no_more_above) goto read_backwards;
  if(no_more_above && !no_more_below) goto read_forward;
  

done:
  LOG_EXIT();
  return;

}

/** 
 * This method is an idea of the path
 * The new decision is that (for the moment) everything that is user-facing
 * should deal ONLY with local-indexing of the file
void LogParserInterface::jumpToGlobalLine(line_t global_line_id){
  LOG_LOGENTRY(5, "LogParserInterface::jumpToGlobalLine");
  LOG_FCT(5, "Jumping to global line %ld\n", global_line_id);
  if(block.lines.front().line_num >= global_line_id && block.lines.back().line_num <= global_line_id){
    return;
  }
  
  block.lines.clear();
  block.raw_lines.clear();
  bool finished = false, no_more_above = false, no_more_below = false;
  size_t segment_lines_left = block_size/2;
  size_t  nread = 0;
  size_t line_storage_id = 0;
  ProcessedLine pl;

  ffr->goToLine(global_line_id);
read_backwards:
  LOG_FCT(5, "Reading backwards (%ld lines max)\n", segment_lines_left);
  while( segment_lines_left > 0  && 
      (nread = ffr->getPreviousValidLine(raw_line_storage + ffr->m_max_chars_per_line*line_storage_id, pl)) != 0){
    block.lines.push_front(pl);
    block.raw_lines.push_front(raw_line_storage + ffr->m_max_chars_per_line*line_storage_id);
    line_storage_id++;
    segment_lines_left--;
  }
  no_more_above = nread == 0;
  segment_lines_left = block_size - line_storage_id;
  finished = (line_storage_id == block_size) || no_more_below;
  if(finished) goto done;
  ffr->goToLine(global_line_id);
read_forward:
  LOG_FCT(5, "Reading forward (%ld lines max)\n", segment_lines_left);
  while( segment_lines_left > 0  && 
      (nread = ffr->getNextValidLine(raw_line_storage + ffr->m_max_chars_per_line*line_storage_id, pl)) != 0){
    block.lines.push_back(pl);
    block.raw_lines.push_back(raw_line_storage + ffr->m_max_chars_per_line*line_storage_id);
    line_storage_id++;
    segment_lines_left--;
  }
  no_more_below = nread == 0;
  segment_lines_left = block_size - line_storage_id;
  finished = (line_storage_id == block_size) || (no_more_above && no_more_below);
  if(finished) goto done;

  // Not finished, either
  // Could read half block above, but not below (try again, above)
  // OR
  // Could read half block below, but not above (try again, below)
  // OR
  // Could not read half block in either direction (doomed, should never happen as file fits into 1 block)

  if(no_more_below && !no_more_above) goto read_backwards;
  if(no_more_above && !no_more_below) goto read_forward;
  

done:
  LOG_EXIT();
  return;
}*/


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