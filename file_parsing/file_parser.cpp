#include "file_parser.hpp"

#include <exception>

ProcessedLine::ProcessedLine(line_t line, std::string_view& sv, Parser* p) {
  set_data(line, sv, p);
}

void  ProcessedLine::set_data(line_t line, std::string_view& sv, Parser* p) {
  line_num = line;
  raw_line = raw_line;
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


  line_blocks[LP_PREV_BLOCK]->resize(block_size);
  line_blocks[LP_MAIN_BLOCK]->resize(block_size);
  line_blocks[LP_NEXT_BLOCK]->resize(block_size);
  tmp_line_block->resize(block_size);

  raw_blocks[0].resize(block_size * 201);
  raw_blocks[1].resize(block_size * 201);
  raw_blocks[2].resize(block_size * 201);

}

FileParser::~FileParser(){
  delete line_blocks[LP_PREV_BLOCK];
  delete line_blocks[LP_MAIN_BLOCK];
  delete line_blocks[LP_NEXT_BLOCK];
  delete tmp_line_block;
}

void FileParser::setLineFormat(LineFormat* lf){
  // TODO
}

void FileParser::setParser(Parser* p){
  parser = p;
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

void FileParser::fillBlock(int which){
  char lcl[2048];
  int glbl_block_id = (curr_main_block_id - 1 + which);
  if(glbl_block_id >= blocks_offsets.size()){
    // We don't know offset, need to read forward
    // TODO we should have a thread filling offsets on file open
    std::streampos fpos = blocks_offsets.back();
    is.seekg(fpos);
    int cbid = blocks_offsets.size() -1;
    while(cbid != glbl_block_id){
      for(int i = 0; i < block_size; i++) is.getline(lcl, 2048);
      cbid++;
    }
  }

  int blockoffset = blocks_offsets[glbl_block_id-1];
  is.seekg(blockoffset);
  int block_line_nbr = getBlockStartLine(which);
  size_t curr_arr_id = 0, next_arr_id = 0;
  size_t tot_size = raw_blocks[which].size();
  for(int i = 0; i < block_size; i++){
    // getline will read until \n 
    // which will be extracted and counted for gcount but not stored
    char* arr_stt = raw_blocks[which].data() + curr_arr_id;
    is.getline(arr_stt, tot_size - curr_arr_id);
    if(is.fail()){
      // TODO
    }
    if(is.eof()) break;
    next_arr_id = curr_arr_id + is.gcount() + 1;
    raw_blocks[which][next_arr_id - 1] = '\0';
    std::string_view sv(arr_stt);
    line_blocks[which]->at(i)->set_data(block_line_nbr + i, sv, parser);
    curr_arr_id = next_arr_id;
  }
      

}

void FileParser::setActiveLine(line_t l){
  active_line = l;
  if(l < getMainBlockStartLine()){
    if(l >= getPrevBlockStartLine()){
      slideBlocksBackward(1);
      curr_main_block_id-=1;
      fillPrevBlock();
    } else if(l >= getPrevBlockStartLine() - block_size) {
      slideBlocksBackward(2);
      curr_main_block_id-=2;
      fillPrevBlock();
      fillMainBlock();
    } else {
      curr_main_block_id = l/block_size;
      fillPrevBlock();
      fillMainBlock();
      fillNextBlock();
    }
  }
  if(l > getMainBlockEndLine()){
    if(l <= getNextBlockEndLine()){
      slideBlocksBackward(1);
      curr_main_block_id+=1;
      fillNextBlock();
    } else if(l <= getNextBlockEndLine() + block_size) {
      slideBlocksBackward(2);
      curr_main_block_id+=2;
      fillMainBlock();
      fillNextBlock();
    } else {
      curr_main_block_id = l/block_size;
      fillPrevBlock();
      fillMainBlock();
      fillNextBlock();
    }
  }
}

std::vector<std::string_view> FileParser::getLines(line_t from, line_t count){

}

std::vector<line_t> FileParser::findOccurences(std::string match, line_t from, bool forward){

}
