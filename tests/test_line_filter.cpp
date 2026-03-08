#include <catch2/catch_test_macros.hpp>

#include "test_helpers.hpp"

// Integration-level filter tests: FieldFilter applied while reading sample.log
// For unit-level filter tests (all comparisons, CombinedFilter, inversion),
// see test_filters.cpp.

TEST_CASE("Basic line filtering") {
  setup();
  std::unique_ptr<LineFormat> lf = getDefaultLineFormat();
  std::shared_ptr<Parser> parser = Parser::fromLineFormat(std::move(lf));

  std::string filename = TEST_FOLDER "data/sample.log";
  std::ifstream file(filename);
  std::string line;

  ParsedLine* pl = new ParsedLine(parser->format.get());

  SECTION("Filter on int") {
    int64_t base_val = 85409;
    std::shared_ptr<LineFilter> filter = std::make_shared<FieldFilter>(parser->format.get(), "Time", FilterComparison::GREATER_EQ, &base_val);
    int i = 1;
    while (std::getline(file, line)) {
      parser->parseLine(line, pl);
      if (i >= 35) {
        REQUIRE(filter->passes(pl));
      } else {
        REQUIRE_FALSE(filter->passes(pl));
      }
      i++;
    }
  }

  SECTION("Filter on string") {
    std::string base_val = "INFO";
    std::shared_ptr<LineFilter> filter = std::make_shared<FieldFilter>(parser->format.get(), "Level", FilterComparison::EQUAL, &base_val);
    int matches = 0;
    while (std::getline(file, line)) {
      parser->parseLine(line, pl);
      if (filter->passes(pl)) matches++;
    }
    REQUIRE(matches == 10);
  }

  delete pl;
  teardown();
}
