#include <catch2/catch_test_macros.hpp>

#include "filtered_file_reader.hpp"
#include "line_format.hpp"
#include "line_parser.hpp"
#include "line_filter.hpp"
#include "processed_line.hpp"
#include "log_parser_interface.hpp"

#include <cstring>
#include <fstream>
#include <iosfwd>
#include <string>
#include <string_view>

extern "C"{
  #include "logging.h"
}

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

std::string_view info_lines[10] = {
  "0322 085338 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085352 INFO   :.......rsvp_parse_objects: obj RSVP_HOP hop=9.67.116.99, lih=0",
  "0322 085352 INFO   :.......rsvp_flow_stateMachine: state RESVED, event RESV",
  "0322 085353 INFO   :......router_forward_getOI: Ioctl to query route entry successful",
  "0322 085353 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085409 INFO   :......router_forward_getOI: Ioctl to query route entry successful",
  "0322 085409 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085422 INFO   :.......rsvp_parse_objects: obj RSVP_HOP hop=9.67.116.99, lih=0",
  "0322 085422 INFO   :.......rsvp_flow_stateMachine: state RESVED, event RESV",
  "0322 085424 INFO   :......router_forward_getOI: Ioctl to query route entry successful"
};

std::string_view info_and_bf_lines[14] = {
  "0322 085338 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085352 INFO   :.......rsvp_parse_objects: obj RSVP_HOP hop=9.67.116.99, lih=0",
  "0322 085352 INFO   :.......rsvp_flow_stateMachine: state RESVED, event RESV",
  "0322 085353 INFO   :......router_forward_getOI: Ioctl to query route entry successful",
  "0x00 0x01 0x02 0x03 ..Da..Ba",
  "0x04 0x05 0x06 0x07 ..Da..Ba",
  "0x08 0x09 0x0A 0x0B ..Da..Ba",
  "0x0C 0x0D 0x0E 0x0F ..Da..Ba",
  "0322 085353 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085409 INFO   :......router_forward_getOI: Ioctl to query route entry successful",
  "0322 085409 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085422 INFO   :.......rsvp_parse_objects: obj RSVP_HOP hop=9.67.116.99, lih=0",
  "0322 085422 INFO   :.......rsvp_flow_stateMachine: state RESVED, event RESV",
  "0322 085424 INFO   :......router_forward_getOI: Ioctl to query route entry successful"
};

void setup(){
  logger_setup();
  logger_set_minlvl(0);
}

void teardown(){
  logger_teardown();
}

TEST_CASE("Line Format specifier parsing"){
  setup();
  std::string spec = "{INT:Date} {INT:Time} {STR:Level} {CHR:, ,1}:{CHR:,.,1}{STR:Source}:{CHR:, ,1}{STR:Mesg}";
  LineFormat *lf = LineFormat::fromFormatString(spec);
  lf->toString();
  REQUIRE(lf->fields.size() == 13);
  REQUIRE(dynamic_cast<LineIntField*>(lf->fields[0]) != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[1]) != nullptr);
  REQUIRE(dynamic_cast<LineIntField*>(lf->fields[2]) != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[3]) != nullptr);
  REQUIRE(dynamic_cast<LineStrField*>(lf->fields[4]) != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[5]) != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[6]) != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[7]) != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[8]) != nullptr);
  REQUIRE(dynamic_cast<LineStrField*>(lf->fields[9]) != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[10]) != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[11]) != nullptr);
  REQUIRE(dynamic_cast<LineStrField*>(lf->fields[12]) != nullptr);

  Parser* p = Parser::fromLineFormat(lf);
  ParsedLine* pl = new ParsedLine(lf);

  REQUIRE(p->parseLine(info_lines[0], pl) == true);
  teardown();
}

TEST_CASE("Basic line parsing"){
  setup();
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
  teardown();
}

TEST_CASE("Basic line filtering"){
  setup();
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
    delete filter;
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
    delete filter;
  }

  for(int i = 0;i < lf->fields.size(); i++){
      delete lf->fields[i];
  }
  delete lf;
  delete pl;
  delete parser;
  teardown();
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
  return -1;
}

TEST_CASE("Basic Filtered File Reader"){
  setup();
  LineFormat* lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log"; 
  // Global so that state is updated between tests, stronger testing :), harder debugging :(
  FilteredFileReader ffr(filename, lf, nullptr, 10);
  ProcessedLine pl;

  SECTION("Filter on string, forward"){
    ffr.m_accept_bad_format = false;
    std::string base_val = "INFO";
    LineFilter* filter = new LineFilter(lf, "Level", FilterComparison::EQUAL, &base_val);
    ffr.setFilter(filter);
    
    char* data =(char*)malloc(ffr.m_max_chars_per_line);
    int count = 0;
    while(ffr.getNextValidLine(data, pl) != 0){
      REQUIRE ( pl.line_num == count_to_info_line(count));
      count++;

    }
    REQUIRE( count == 10);
    free(data);
    delete filter;
  }

  SECTION("Filter on string, backwards"){
    ffr.m_accept_bad_format = false;
    std::string base_val = "INFO";
    LineFilter* filter = new LineFilter(lf, "Level", FilterComparison::EQUAL, &base_val);
    ffr.setFilter(filter);
    
    char* data = (char*)malloc(ffr.m_max_chars_per_line);
    
    ffr.skipNextRawLines(1000); // Go to eof
    
    int count = 9;
    while(ffr.getPreviousValidLine(data, pl) != 0){
      REQUIRE ( pl.line_num == count_to_info_line(count));
      count--;

    }
    REQUIRE( count == -1);
    free(data);
    delete filter;
  }

  SECTION("Filter on string, forward then backwards"){
    ffr.m_accept_bad_format = false;
    std::string base_val = "INFO";
    LineFilter* filter = new LineFilter(lf, "Level", FilterComparison::EQUAL, &base_val);
    ffr.setFilter(filter);
    
    char* data = (char*)malloc(ffr.m_max_chars_per_line);
    ffr.seekRawLine(0);
    int count = 0;
    while(ffr.getNextValidLine(data, pl) != 0){
      REQUIRE ( pl.line_num == count_to_info_line(count));
      count++;
    }
    REQUIRE( count == 10);
    while(ffr.getPreviousValidLine(data, pl) != 0){
      count--;
      REQUIRE ( pl.line_num == count_to_info_line(count));
    }
    REQUIRE( count == 0 );
    free(data);
    delete filter;
  }

  SECTION("Filter on string, backwards then forward"){
    ffr.m_accept_bad_format = false;
    std::string base_val = "INFO";
    LineFilter* filter = new LineFilter(lf, "Level", FilterComparison::EQUAL, &base_val);
    ffr.setFilter(filter);
    
    char* data = (char*)malloc(ffr.m_max_chars_per_line);
    
    ffr.skipNextRawLines(1000); // Go to eof
    
    int count = 9;
    while(ffr.getPreviousValidLine(data, pl) != 0){
      LOG(1, "Got previous line %llu %s\n", pl.line_num, data);
      REQUIRE ( pl.line_num == count_to_info_line(count));
      count--;
    }
    count++;
    REQUIRE( count == 0);
    while(ffr.getNextValidLine(data, pl) != 0){
      LOG(1, "Got next line %llu %s\n", pl.line_num, data);
      REQUIRE ( pl.line_num == count_to_info_line(count));
      count++;
    }
    REQUIRE( count == 10);
    free(data);
    delete filter;
  }
  for(int i = 0;i < lf->fields.size(); i++){
    delete lf->fields[i];
  }
  delete lf;
  teardown();
}



TEST_CASE("Testing interface"){
  setup();
  LineFormat* lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log"; 
  std::string base_val = "INFO";
  LineFilter* filter = new LineFilter(lf, "Level", FilterComparison::EQUAL, &base_val);
  LogParserInterface lpi(filename, lf, filter, 4);
  
  std::srand(std::time({}));
  for(int i = 0; i < 100; i++){
    int lineid = std::rand() % 10;
    std::string_view line = lpi.getLine(lineid).line->raw_line;
    REQUIRE( std::strcmp(line.data(), info_and_bf_lines[lineid].data()) == 0);
  }
  


  for(int i = 0;i < lf->fields.size(); i++){
    delete lf->fields[i];
  }
  delete lf;
  delete filter;
  teardown();
}