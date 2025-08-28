#include "file_parser.hpp"

#include <exception>

ProcessedLine::ProcessedLine(line_t line, char* s, size_t n_char, Parser* p) {
  set_data(line, s, n_char, p);
}

void  ProcessedLine::set_data(line_t line, char* s, size_t n_char, Parser* p) {
  line_num = line;
  raw_line = std::string_view(s, n_char);
  if(p->format == nullptr){
    pl = nullptr;
    well_formated = false;
  } else {
    pl = new ParsedLine(p->format);
    well_formated = p->parseLine(raw_line, pl);
  }

}

FilteredFileReader::FilteredFileReader(std::string& fname, LineFormat* lf) 
  : m_max_chars_per_line(256), m_lf(lf){

  m_is = std::ifstream(fname, std::ios::in);
  m_line_parser = Parser::fromLineFormat(m_lf);
  m_filter = nullptr;
  m_checkpoints.push_back(m_is.tellg());
  m_curr_line = 0;
}

FilteredFileReader::FilteredFileReader(std::string& fname, LineFormat* lf, LineFilter* filter)
  : FilteredFileReader(fname, lf){
  m_filter = filter;
}

#define TRASH_SIZE 1024
static char trash[TRASH_SIZE];

void FilteredFileReader::incrCurrLine(){
  m_curr_line++;
  if(m_curr_line % m_checkpoint_dist == 0){
    line_t ncp = m_curr_line / m_checkpoint_dist;
    // Could do sanity check checkpoint[ncp] == is.tellg()
    if(ncp == m_checkpoints.size()){
      m_checkpoints.push_back(m_is.tellg());
    } 
    if(ncp > m_checkpoints.size()){
      // Something went very wrong
    }
    
  }
}

size_t FilteredFileReader::readRawLine(char* s, uint32_t max_chars){
  m_is.getline(s, max_chars);
  // if line is "aaa\n" then "aaa\0" will be stored in s, but gcount still return 4
  size_t nread = m_is.gcount();
  while(m_is.fail()){
    m_is.getline(trash, TRASH_SIZE);
  }
  incrCurrLine();
  return nread;
}

void FilteredFileReader::skipNextRawLines(line_t n){
  
  while(n){
    m_is.getline(trash, TRASH_SIZE);
    // if line is > than 1024 char, getline sets failbit
    if(m_is.fail()) continue;
    n--;
    incrCurrLine();
  }
}

void FilteredFileReader::addFilteredOutGroup(line_t stt, line_t end, uint64_t idx){
  // Want to just insert but might need to merge left/right/both
  bool need_left_merge = idx > 0 && stt <= m_filtered_out_lines[idx-1].second + 1;
  bool need_right_merge = idx < m_filtered_out_lines.size() - 1 && end >= m_filtered_out_lines[idx].first - 1;

  if(need_left_merge && need_right_merge){
    m_filtered_out_lines[idx-1].second = m_filtered_out_lines[idx].second;
    m_filtered_out_lines.erase(m_filtered_out_lines.begin() + idx);
  } else if (need_left_merge) {
    m_filtered_out_lines[idx-1].second = end;
  } else if (need_right_merge) {
    m_filtered_out_lines[idx].first = stt;
  } else {
    m_filtered_out_lines.emplace(m_filtered_out_lines.begin() + idx, std::make_pair(stt, end));
  }
  
}

void FilteredFileReader::seekRawLine(line_t num){
  line_t ncp = num / m_checkpoint_dist;
  line_t curr = 0;
  if(ncp < m_checkpoints.size()){
    m_is.seekg(m_checkpoints[ncp]);
    m_curr_line = m_checkpoint_dist * ncp;
  } else {
    // TODO dont go back if line is in current block
    m_is.seekg(m_checkpoints.back());
    m_curr_line = m_checkpoint_dist *(m_checkpoints.size()-1);
  }
  skipNextRawLines(num-curr);
}

size_t FilteredFileReader::getNextValidLine(char* dest, ProcessedLine& pl){
  // TODO Binary search
  int i = 0;
  while(i < m_filtered_out_lines.size() && m_curr_line > m_filtered_out_lines[i].second){
    i++;
  }
  if(i == m_filtered_out_lines.size()){
    // We are exploring uncharted territory in the file
    // Cannot move forward
  } else {
    // here we know that curr_line <= filtered_lines[i].end AND curr_line > filtered_line[i-1].end
    if(m_curr_line >= m_filtered_out_lines[i].first){
      // Here is a block of line, that was already processed once and we know doesnt pass the filter
      // Just skip ahead
      seekRawLine(m_filtered_out_lines[i].second + 1);
      
      // used later
      i++;
    } else {
      // We are on a line that doesn't belong to any known filter_out groups
      // Cannot move forward
    }    
  }
  line_t fo_end, fo_begin = m_curr_line;
  while(!m_is.eof()){
    fo_end = m_curr_line -1;
    size_t nread = readRawLine(dest, m_max_chars_per_line);
    pl.set_data(m_curr_line-1, dest, nread, m_line_parser);
    if( (m_accept_bad_format && !pl.well_formated) 
        || m_filter == nullptr || m_filter->passes(pl.pl)){
      
          if(fo_end > fo_begin){ // Since intervals are [incl; incl], could add new block on ">=", but let's not care about single lines
        addFilteredOutGroup(fo_begin, fo_end, i);
      }
      return nread;
    }
  }
  return 0;
}

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
  // TODO
}

void LogParserInterface::setFilter(std::shared_ptr<LineFilter> lf){
  // TODO
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

std::vector<std::string_view> LogParserInterface::getLines(line_t from, line_t count){

}

line_t LogParserInterface::findNextOccurence(std::string match, line_t from, bool forward){

}
