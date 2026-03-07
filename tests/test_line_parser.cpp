#include <catch2/catch_test_macros.hpp>

#include "test_helpers.hpp"

TEST_CASE("Basic line parsing") {
  setup();
  LineFormat* lf = getDefaultLineFormat();
  Parser* parser = Parser::fromLineFormat(lf);

  std::string filename = TEST_FOLDER "data/sample.log";
  std::ifstream file(filename);
  std::string line;

  ParsedLine* pl = new ParsedLine(lf);
  int i = 0;
  while (std::getline(file, line)) {
    bool success = parser->parseLine(line, pl);
    if (i >= 25 && i <= 28) {
      REQUIRE_FALSE(success);
    } else {
      REQUIRE(success);
    }
    if (i == 0) {
      REQUIRE(*(pl->getIntField(0)) == 322);
      REQUIRE(*(pl->getChrField(0)) == ' ');
      REQUIRE(*(pl->getIntField(1)) == 85338);
      REQUIRE(*(pl->getStrField(0)) == std::string_view("TRACE"));
      REQUIRE(*(pl->getStrField(1)) == std::string_view("router_forward_getOI"));
      REQUIRE(*(pl->getStrField(2)) == std::string_view("source address:   9.67.116.98"));
    }
    i++;
  }
  // Verify last line's content
  REQUIRE(*(pl->getIntField(0)) == 322);
  REQUIRE(*(pl->getChrField(0)) == ' ');
  REQUIRE(*(pl->getIntField(1)) == 85424);
  REQUIRE(*(pl->getStrField(0)) == std::string_view("TRACE"));
  REQUIRE(*(pl->getStrField(1)) == std::string_view("router_forward_getOI"));
  REQUIRE(*(pl->getStrField(2)) == std::string_view("route handle:   LASTLINE"));

  delete pl;
  delete parser;
  freeLineFormat(lf);
  teardown();
}
