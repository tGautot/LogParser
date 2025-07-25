#include <iostream>
#include <fstream>
#include <string>

#include "line_parser.hpp"
#include "line_filter.hpp"


int main(int argc, char** argv){
  if(argc == 1){
    std::cout << "Specify a file" << std::endl;
    return 1;
  }


  LineFormat lf;
  lf.addField(new LineIntField("Date"));
  lf.addField(new LineChrField("", ' ', true));
  lf.addField(new LineIntField("Time"));
  lf.addField(new LineChrField("", ' ', true));
  lf.addField(new LineStrField("Level", StrFieldStopType::DELIM, ' ', 0));
  lf.addField(new LineChrField("", ' ', true));
  lf.addField(new LineChrField("", ':', false));
  lf.addField(new LineChrField("", '.', true));
  lf.addField(new LineStrField("Source", StrFieldStopType::DELIM, ':', 0));
  lf.addField(new LineChrField("", ':', false));
  lf.addField(new LineStrField("Mesg", StrFieldStopType::DELIM, 0, 0));

  Parser* parser = Parser::fromLineFormat(&lf);
 
  char* trace_level = "TRACE";
  LineFilter loglevel_filter(&lf, "Level", FilterComparison::EQUAL, (void*)(&trace_level), false);

  std::ifstream file(argv[1]);
  std::string line;
  ParsedLine* pl = new ParsedLine(lf);
  while(std::getline(file, line)){
    bool success = parser->parseLine(line, pl);
    std::cout << "PL results: ";
    if(!success){
      std::cout << " failure" << std::endl;
    }
    pl->asStringToStream(std::cout);
    std::cout << "\n\tPasses TRACE filter: " << loglevel_filter.passes(pl) << std::endl;
  }




}
