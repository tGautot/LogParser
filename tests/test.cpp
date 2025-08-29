#include <catch2/catch_test_macros.hpp>

#include "filtered_file_reader.hpp"
#include "line_format.hpp"
#include "line_parser.hpp"
#include "line_filter.hpp"
#include "processed_line.hpp"

#include <fstream>
#include <iosfwd>
#include <string>
#include <string_view>

#define TEST_FOLDER "../../tests/"

LineFormat* getDefaultLineFormat(){
  LineFormat* lf = new LineFormat();
  lf->addField(new LineIntField("Date"));
  lf->addField(new LineChrField("", ' ', true));
  lf->addField(new LineIntField("Time"));
  lf->addField(new LineChrField("", ' ', true));
  lf->addField(new LineStrField("Level", StrFieldStopType::DELIM, ' ', 0));
  lf->addField(new LineChrField("", ' ', true));
  lf->addField(new LineChrField("", ':', false));
  lf->addField(new LineChrField("", '.', true));
  lf->addField(new LineStrField("Source", StrFieldStopType::DELIM, ':', 0));
  lf->addField(new LineChrField("", ':', false));
  lf->addField(new LineChrField("", ' ', true));
  lf->addField(new LineStrField("Mesg", StrFieldStopType::DELIM, 0, 0));
  return lf;
}


TEST_CASE("Basic line parsing"){
  LineFormat* lf = getDefaultLineFormat();

  Parser* parser = Parser::fromLineFormat(lf);

  std::string filename = TEST_FOLDER "data/sample.log"; 
  std::ifstream file(filename);
  std::string line;
  
  ParsedLine* pl = new ParsedLine(lf);
  int i = 0;
  while(std::getline(file, line)){
    bool success = parser->parseLine(line, pl);
    if(i >= 25 && i <= 28){
      REQUIRE_FALSE( success );
    } else {
      REQUIRE( success );
    }
    if(i == 0){
      // Verify first line's content
      REQUIRE( *(pl->getIntField(0)) == 322);
      REQUIRE( *(pl->getChrField(0)) == ' ');
      REQUIRE( *(pl->getIntField(1)) == 85338);
      REQUIRE( *(pl->getStrField(0)) == std::string_view("TRACE"));
      REQUIRE( *(pl->getStrField(1)) == std::string_view("router_forward_getOI"));
      REQUIRE( *(pl->getStrField(2)) == std::string_view("source address:   9.67.116.98"));
    }

    i++;
  }
  // Verify last line's content
  REQUIRE( *(pl->getIntField(0)) == 322);
  REQUIRE( *(pl->getChrField(0)) == ' ');
  REQUIRE( *(pl->getIntField(1)) == 85424);
  REQUIRE( *(pl->getStrField(0)) == std::string_view("TRACE"));
  REQUIRE( *(pl->getStrField(1)) == std::string_view("router_forward_getOI"));
  REQUIRE( *(pl->getStrField(2)) == std::string_view("route handle:   LASTLINE"));
  delete pl;
  for(int i = 0;i < lf->fields.size(); i++){
    delete lf->fields[i];
  }
  delete lf;
  delete parser;
}

TEST_CASE("Basic line filtering"){
  LineFormat* lf = getDefaultLineFormat();

  Parser* parser = Parser::fromLineFormat(lf);

  std::string filename = TEST_FOLDER "data/sample.log"; 
  std::ifstream file(filename);
  std::string line;
  
  ParsedLine* pl = new ParsedLine(lf);
  
  SECTION("Filter on int"){
    int64_t base_val = 85409;
    LineFilter* filter = new LineFilter(lf, "Time", FilterComparison::GREATER_EQ, &base_val);
    int i = 1;
    while(std::getline(file, line)){
      bool success = parser->parseLine(line, pl);
      if(i >= 35){
        REQUIRE( filter->passes(pl));
      } else {
        REQUIRE_FALSE( filter->passes(pl) );
      }
      i++;
    }
  }

  SECTION("Filter on string"){
    std::string base_val = "INFO";
    LineFilter* filter = new LineFilter(lf, "Level", FilterComparison::EQUAL, &base_val);
    int matches = 0;
    while(std::getline(file, line)){
      bool success = parser->parseLine(line, pl);
      if(filter->passes(pl)){
        matches++;
      }
    }
    REQUIRE( matches == 10 );
  }

  for(int i = 0;i < lf->fields.size(); i++){
      delete lf->fields[i];
  }
  delete lf;
  delete pl;
  delete parser;
}

int count_to_info_line(int id){
  switch(id){
    case 0:
      return 4;
    case 1:
      return 12;
    case 2:
      return 14;
    case 3:
      return 20;
    case 4:
      return 29;
    case 5:
      return 36;
    case 6:
      return 41;
    case 7:
      return 49;
    case 8:
      return 51;
    case 9:
      return 57;
  }
}

TEST_CASE("Basic Filtered File Reader"){
  LineFormat* lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log"; 
  // Global so that state is updated between tests, stronger testing :), harder debugging :(
  FilteredFileReader ffr(filename, lf);
  ProcessedLine pl;

  SECTION("Filter on string, forward"){
    ffr.m_accept_bad_format = false;
    std::string base_val = "INFO";
    LineFilter* filter = new LineFilter(lf, "Level", FilterComparison::EQUAL, &base_val);
    ffr.m_filter = filter;
    
    std::vector<char*> data;
    data.push_back((char*)malloc(ffr.m_max_chars_per_line));
    int count = 0;
    while(ffr.getNextValidLine(data.back(), pl) != 0){
      REQUIRE ( pl.line_num == count_to_info_line(count));
      count++;

    }
    REQUIRE( count == 10);

  }

  SECTION("Filter on string, backwards"){
    ffr.m_accept_bad_format = false;
    std::string base_val = "INFO";
    LineFilter* filter = new LineFilter(lf, "Level", FilterComparison::EQUAL, &base_val);
    ffr.m_filter = filter;
    
    std::vector<char*> data;
    data.push_back((char*)malloc(ffr.m_max_chars_per_line));
    
    ffr.skipNextRawLines(1000); // Go to eof
    
    int count = 9;
    while(ffr.getPreviousValidLine(data.back(), pl) != 0){
      REQUIRE ( pl.line_num == count_to_info_line(count));
      count--;

    }
    REQUIRE( count == -1);

  }

  SECTION("Filter on string, forward then backwards"){
    ffr.m_accept_bad_format = false;
    std::string base_val = "INFO";
    LineFilter* filter = new LineFilter(lf, "Level", FilterComparison::EQUAL, &base_val);
    ffr.m_filter = filter;
    
    std::vector<char*> data;
    data.push_back((char*)malloc(ffr.m_max_chars_per_line));
    ffr.seekRawLine(0);
    int count = 0;
    while(ffr.getNextValidLine(data.back(), pl) != 0){
      REQUIRE ( pl.line_num == count_to_info_line(count));
      count++;
    }
    REQUIRE( count == 10);
    while(ffr.getPreviousValidLine(data.back(), pl) != 0){
      count--;
      REQUIRE ( pl.line_num == count_to_info_line(count));
    }
    REQUIRE( count == 0 );

  }

  SECTION("Filter on string, backwards then forward"){
    ffr.m_accept_bad_format = false;
    std::string base_val = "INFO";
    LineFilter* filter = new LineFilter(lf, "Level", FilterComparison::EQUAL, &base_val);
    ffr.m_filter = filter;
    
    std::vector<char*> data;
    data.push_back((char*)malloc(ffr.m_max_chars_per_line));
    
    ffr.skipNextRawLines(1000); // Go to eof
    
    int count = 9;
    while(ffr.getPreviousValidLine(data.back(), pl) != 0){
      REQUIRE ( pl.line_num == count_to_info_line(count));
      count--;
    }
    count++;
    REQUIRE( count == 0);
    while(ffr.getNextValidLine(data.back(), pl) != 0){
      REQUIRE ( pl.line_num == count_to_info_line(count));
      count++;
    }
    REQUIRE( count == 10);

  }
}