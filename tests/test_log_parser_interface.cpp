#include <catch2/catch_test_macros.hpp>

#include "test_helpers.hpp"

#include <cstdlib>
#include <ctime>

TEST_CASE("Testing interface") {
  setup();
  LineFormat* lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log";
  std::string base_val = "INFO";
  std::shared_ptr<LineFilter> filter = std::make_shared<FieldFilter>(lf, "Level", FilterComparison::EQUAL, &base_val);
  LogParserInterface lpi(filename, lf, filter, 4);

  std::srand(std::time({}));
  for (int i = 0; i < 100; i++) {
    int lineid = std::rand() % 10;
    std::string_view line = lpi.getLine(lineid).line->raw_line;
    REQUIRE(std::strcmp(line.data(), info_and_bf_lines[lineid].data()) == 0);
  }

  freeLineFormat(lf);
  teardown();
}
