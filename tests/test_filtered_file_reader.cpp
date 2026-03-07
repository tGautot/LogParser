#include <catch2/catch_test_macros.hpp>

#include "test_helpers.hpp"

TEST_CASE("Basic Filtered File Reader") {
  setup();
  LineFormat* lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log";
  // Shared across sections so that state carries over — stronger testing,
  // harder to debug in isolation.
  FilteredFileReader ffr(filename, lf, nullptr, 10);
  ProcessedLine pl;

  SECTION("Filter on string, forward") {
    ffr.m_accept_bad_format = false;
    std::string base_val = "INFO";
    std::shared_ptr<LineFilter> filter = std::make_shared<FieldFilter>(lf, "Level", FilterComparison::EQUAL, base_val);
    ffr.setFilter(filter);

    char* data = (char*)malloc(ffr.m_max_chars_per_line);
    int count = 0;
    while (ffr.getNextValidLine(data, pl) != 0) {
      std::cout << "Got next valid line: " << data << std::endl;
      REQUIRE(pl.line_num == (line_t)count_to_info_line(count));
      count++;
    }
    REQUIRE(count == 10);
    free(data);
  }

  SECTION("Filter on string, backwards") {
    ffr.m_accept_bad_format = false;
    std::string base_val = "INFO";
    std::shared_ptr<LineFilter> filter = std::make_shared<FieldFilter>(lf, "Level", FilterComparison::EQUAL, &base_val);
    ffr.setFilter(filter);

    char* data = (char*)malloc(ffr.m_max_chars_per_line);
    ffr.skipNextRawLines(1000); // go to EOF

    int count = 9;
    while (ffr.getPreviousValidLine(data, pl) != 0) {
      REQUIRE(pl.line_num == (line_t)count_to_info_line(count));
      count--;
    }
    REQUIRE(count == -1);
    free(data);
  }

  SECTION("Filter on string, forward then backwards") {
    ffr.m_accept_bad_format = false;
    std::string base_val = "INFO";
    std::shared_ptr<LineFilter> filter = std::make_shared<FieldFilter>(lf, "Level", FilterComparison::EQUAL, &base_val);
    ffr.setFilter(filter);

    char* data = (char*)malloc(ffr.m_max_chars_per_line);
    ffr.seekRawLine(0);
    int count = 0;
    while (ffr.getNextValidLine(data, pl) != 0) {
      REQUIRE(pl.line_num == (line_t)count_to_info_line(count));
      count++;
    }
    REQUIRE(count == 10);
    while (ffr.getPreviousValidLine(data, pl) != 0) {
      count--;
      REQUIRE(pl.line_num == (line_t)count_to_info_line(count));
    }
    REQUIRE(count == 0);
    free(data);
  }

  SECTION("Filter on string, backwards then forward") {
    ffr.m_accept_bad_format = false;
    std::string base_val = "INFO";
    std::shared_ptr<LineFilter> filter = std::make_shared<FieldFilter>(lf, "Level", FilterComparison::EQUAL, &base_val);
    ffr.setFilter(filter);

    char* data = (char*)malloc(ffr.m_max_chars_per_line);
    ffr.skipNextRawLines(1000); // go to EOF

    int count = 9;
    while (ffr.getPreviousValidLine(data, pl) != 0) {
      LOG(1, "Got previous line %llu %s\n", pl.line_num, data);
      REQUIRE(pl.line_num == (line_t)count_to_info_line(count));
      count--;
    }
    count++;
    REQUIRE(count == 0);
    while (ffr.getNextValidLine(data, pl) != 0) {
      LOG(1, "Got next line %llu %s\n", pl.line_num, data);
      REQUIRE(pl.line_num == (line_t)count_to_info_line(count));
      count++;
    }
    REQUIRE(count == 10);
    free(data);
  }

  freeLineFormat(lf);
  teardown();
}
