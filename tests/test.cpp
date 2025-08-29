#include <catch2/catch_test_macros.hpp>

#include "line_format.hpp"
#include "line_parser.hpp"
#include "line_filter.hpp"

#include <fstream>
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
  REQUIRE( *(pl->getIntField(0)) == 322LL);
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