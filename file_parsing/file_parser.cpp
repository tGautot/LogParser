#include "file_parser.hpp"

#include <exception>

ProcessedLine::ProcessedLine(line_t line, std::string_view sv, Parser* p) : line_num(line), raw_line(sv) {
  if(p->format == nullptr){
    pl = nullptr;
    well_formated = false;
  } else {
    pl = new ParsedLine(p->format);
    well_formated = p->parseLine(sv, pl);
  }

}

FileParser::FileParser(std::string fname, int bsize) : block_size(bsize), line_format(nullptr), active_line(LP_PREV_BLOCK){
  is = std::ifstream(fname, std::ios::in);
  if(is.fail()){
    throw new std::runtime_error("Failed to open file " + fname);
  }
  
  blocks_offsets.push_back(0);

  curr_main_block_id = 0;

  line_blocks[LP_PREV_BLOCK] = new std::vector<ProcessedLine*>();
  line_blocks[LP_MAIN_BLOCK] = new std::vector<ProcessedLine*>();
  line_blocks[LP_NEXT_BLOCK] = new std::vector<ProcessedLine*>();
  tmp_line_block = new std::vector<ProcessedLine*>();


  line_blocks[LP_PREV_BLOCK]->reserve(block_size);
  line_blocks[LP_MAIN_BLOCK]->reserve(block_size);
  line_blocks[LP_NEXT_BLOCK]->reserve(block_size);
  tmp_line_block->reserve(block_size);

}

FileParser::~FileParser(){
  delete line_blocks[LP_PREV_BLOCK];
  delete line_blocks[LP_MAIN_BLOCK];
  delete line_blocks[LP_NEXT_BLOCK];
  delete tmp_line_block;
}

void FileParser::slideBlocksForward(int one_or_two){
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

void FileParser::slideBlocksBackward(int one_or_two){
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

void FileParser::addFilter(std::shared_ptr<LineFilter> lf){
  filters.push_back(lf);
}

void FileParser::clearFilters(){
  filters.clear();
}



std::vector<std::string_view> getLines(line_t from, line_t count){

}

std::vector<line_t> findOccurences(std::string match, line_t from);
