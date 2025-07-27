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

FileParser::FileParser(std::string fname, int bsize) : block_size(bsize), line_format(nullptr), active_line(0){
  is = std::ifstream(fname, std::ios::in);
  if(is.fail()){
    throw new std::runtime_error("Failed to open file " + fname);
  }
  line_blocks[0].reserve(block_size);
  line_blocks[1].reserve(block_size);
  line_blocks[2].reserve(block_size);
  tmp_line_block.reserve(block_size);

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
