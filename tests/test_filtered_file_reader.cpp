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

// ──────────────────────────────────────────────
// getNextValidLine – stop_at_line contract
// ──────────────────────────────────────────────

TEST_CASE("FilteredFileReader::getNextValidLine - stop_at_line is an exclusive upper bound") {
  setup();
  LineFormat* lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log";
  std::string val = "INFO";
  auto filter = std::make_shared<FieldFilter>(lf, "Level", FilterComparison::EQUAL, &val);

  FilteredFileReader ffr(filename, lf, filter, 10);
  ffr.m_accept_bad_format = false;
  char* buf = (char*)malloc(ffr.getMaxCharsPerLine());
  ProcessedLine pl;

  // INFO lines are at raw lines: 4, 12, 14, 20, ...  (0-indexed)

  SECTION("stop_at_line equal to a matching line's number excludes that line") {
    // stop_at_line=4: line 4 is the boundary, so it must NOT be returned.
    REQUIRE(ffr.getNextValidLine(buf, pl, count_to_info_line(0)) == 0);
  }

  SECTION("stop_at_line one past a matching line includes it") {
    // stop_at_line=5 allows line 4 through; nothing else matches before 5.
    REQUIRE(ffr.getNextValidLine(buf, pl, count_to_info_line(0) + 1) != 0);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(0));
    REQUIRE(ffr.getNextValidLine(buf, pl, count_to_info_line(0) + 1) == 0);
  }

  SECTION("stop_at_line spanning N matching lines returns exactly N") {
    // stop_at_line = line[2] + 1 → lines 0, 1, 2 pass; line 3 does not.
    line_t limit = (line_t)count_to_info_line(2) + 1;
    int count = 0;
    while (ffr.getNextValidLine(buf, pl, limit) != 0) count++;
    REQUIRE(count == 3);
  }

  SECTION("stop_at_line=0 returns nothing") {
    REQUIRE(ffr.getNextValidLine(buf, pl, 0) == 0);
  }

  SECTION("returns 0 once all lines are exhausted, and stays at 0") {
    while (ffr.getNextValidLine(buf, pl) != 0) {}
    REQUIRE(ffr.getNextValidLine(buf, pl) == 0);
  }

  free(buf);
  freeLineFormat(lf);
  teardown();
}

// ──────────────────────────────────────────────
// getPreviousValidLine – boundary behaviour
// ──────────────────────────────────────────────

TEST_CASE("FilteredFileReader::getPreviousValidLine - returns 0 at beginning of file") {
  setup();
  LineFormat* lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log";
  std::string val = "INFO";
  auto filter = std::make_shared<FieldFilter>(lf, "Level", FilterComparison::EQUAL, &val);

  FilteredFileReader ffr(filename, lf, filter, 10);
  ffr.m_accept_bad_format = false;
  char* buf = (char*)malloc(ffr.getMaxCharsPerLine());
  ProcessedLine pl;

  // At the very start there is no previous line.
  REQUIRE(ffr.getPreviousValidLine(buf, pl) == 0);

  free(buf);
  freeLineFormat(lf);
  teardown();
}

// ──────────────────────────────────────────────
// seekRawLine – repositions the reader
// ──────────────────────────────────────────────

TEST_CASE("FilteredFileReader::seekRawLine - repositions the reader") {
  setup();
  LineFormat* lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log";
  std::string val = "INFO";
  auto filter = std::make_shared<FieldFilter>(lf, "Level", FilterComparison::EQUAL, &val);

  FilteredFileReader ffr(filename, lf, filter, 10);
  ffr.m_accept_bad_format = false;
  char* buf = (char*)malloc(ffr.getMaxCharsPerLine());
  ProcessedLine pl;

  SECTION("seekRawLine(0) after reading restarts iteration from the first line") {
    ffr.getNextValidLine(buf, pl);
    ffr.getNextValidLine(buf, pl);
    ffr.seekRawLine(0);
    ffr.getNextValidLine(buf, pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(0));
  }

  SECTION("seekRawLine to a matching line returns that line first") {
    ffr.seekRawLine(count_to_info_line(3)); // raw line 20, which is an INFO line
    ffr.getNextValidLine(buf, pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(3));
  }

  SECTION("seekRawLine to a non-matching line advances to the next valid line") {
    // Seek one past the first INFO line (raw line 4 → seek to 5).
    // The next INFO line after that is count_to_info_line(1).
    ffr.seekRawLine((line_t)count_to_info_line(0) + 1);
    ffr.getNextValidLine(buf, pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(1));
  }

  SECTION("seekRawLine then getPreviousValidLine returns 0 when nothing precedes seek point") {
    ffr.seekRawLine(count_to_info_line(0)); // seek to exactly the first INFO line
    // Nothing valid before this point.
    REQUIRE(ffr.getPreviousValidLine(buf, pl) == 0);
  }

  free(buf);
  freeLineFormat(lf);
  teardown();
}

// ──────────────────────────────────────────────
// setFilter – changes active filter and resets position
// ──────────────────────────────────────────────

TEST_CASE("FilteredFileReader::setFilter - changes the active filter and resets position") {
  setup();
  LineFormat* lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log";

  FilteredFileReader ffr(filename, lf, nullptr, 10);
  ffr.m_accept_bad_format = false;
  char* buf = (char*)malloc(ffr.getMaxCharsPerLine());
  ProcessedLine pl;

  SECTION("setFilter resets iteration to the beginning of the file") {
    std::string v = "INFO";
    ffr.setFilter(std::make_shared<FieldFilter>(lf, "Level", FilterComparison::EQUAL, &v));
    ffr.getNextValidLine(buf, pl); // consume first INFO line
    ffr.getNextValidLine(buf, pl); // consume second INFO line
    REQUIRE(pl.line_num > (line_t)count_to_info_line(0));

    std::string v2 = "INFO";
    ffr.setFilter(std::make_shared<FieldFilter>(lf, "Level", FilterComparison::EQUAL, &v2));
    ffr.getNextValidLine(buf, pl); // should be back at the first INFO line
    REQUIRE(pl.line_num == (line_t)count_to_info_line(0));
  }

  SECTION("setFilter(nullptr) removes the filter; all lines are returned") {
    std::string v = "INFO";
    ffr.setFilter(std::make_shared<FieldFilter>(lf, "Level", FilterComparison::EQUAL, &v));
    int with_filter = 0;
    while (ffr.getNextValidLine(buf, pl) != 0) with_filter++;
    REQUIRE(with_filter == 10);

    ffr.setFilter(nullptr);
    int without_filter = 0;
    while (ffr.getNextValidLine(buf, pl) != 0) without_filter++;
    REQUIRE(without_filter > with_filter);
  }

  free(buf);
  freeLineFormat(lf);
  teardown();
}

// ──────────────────────────────────────────────
// setFormat – switches the active parsing format
//
// NOTE: these tests document INTENDED post-refactor behaviour.
// The current implementation updates m_lf but does not rebuild
// the internal Parser, so they are expected to fail until the
// refactor is complete.
// ──────────────────────────────────────────────

TEST_CASE("FilteredFileReader::setFormat - switches the active parsing format") {
  setup();
  std::string filename = TEST_FOLDER "data/sample.log";

  // Correct format: matches the log lines, all well-formatted.
  LineFormat* lf_correct = getDefaultLineFormat();

  // Wrong format: five consecutive INT fields — will fail to parse log lines.
  LineFormat* lf_wrong = LineFormat::fromFormatString("{INT:A} {INT:B} {INT:C} {INT:D} {INT:E}");

  FilteredFileReader ffr(filename, lf_correct, nullptr, 10);
  char* buf = (char*)malloc(ffr.getMaxCharsPerLine());
  ProcessedLine pl;

  SECTION("with correct format lines parse successfully") {
    ffr.getNextValidLine(buf, pl);
    REQUIRE(pl.well_formated == true);
  }

  SECTION("after setFormat(wrong), lines fail to parse") {
    ffr.setFormat(lf_wrong);
    ffr.seekRawLine(0);
    ffr.getNextValidLine(buf, pl);
    REQUIRE(pl.well_formated == false);
  }

  SECTION("after setFormat(wrong) then setFormat(correct), lines parse again") {
    ffr.setFormat(lf_wrong);
    ffr.seekRawLine(0);
    ffr.getNextValidLine(buf, pl);
    REQUIRE(pl.well_formated == false);

    LineFormat* lf_correct2 = getDefaultLineFormat();
    ffr.setFormat(lf_correct2);
    ffr.seekRawLine(0);
    ffr.getNextValidLine(buf, pl);
    REQUIRE(pl.well_formated == true);
    freeLineFormat(lf_correct2);
  }

  free(buf);
  freeLineFormat(lf_correct);
  freeLineFormat(lf_wrong);
  teardown();
}

// ──────────────────────────────────────────────
// goToPosition – repositions using a stored stream position
// ──────────────────────────────────────────────

TEST_CASE("FilteredFileReader::goToPosition - repositions using a stored stream position") {
  setup();
  LineFormat* lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log";
  std::string val = "INFO";
  auto filter = std::make_shared<FieldFilter>(lf, "Level", FilterComparison::EQUAL, &val);

  FilteredFileReader ffr(filename, lf, filter, 10);
  ffr.m_accept_bad_format = false;
  char* buf = (char*)malloc(ffr.getMaxCharsPerLine());
  char* buf2 = (char*)malloc(ffr.getMaxCharsPerLine());
  ProcessedLine pl;

  SECTION("getNextValidLine after goToPosition returns the same line") {
    ProcessedLine info0;
    ffr.getNextValidLine(buf2, info0);  // INFO line 0 at raw line 4
    ffr.getNextValidLine(buf, pl);      // advance past it

    ffr.goToPosition(info0.stt_pos, info0.line_num);
    ffr.getNextValidLine(buf, pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(0));
  }

  SECTION("getPreviousValidLine after goToPosition returns lines before the position") {
    ProcessedLine info1;
    ffr.getNextValidLine(buf, pl);      // INFO line 0 (raw line 4)
    ffr.getNextValidLine(buf2, info1);  // INFO line 1 (raw line 12)

    // Seek to start of INFO line 1; previous valid line should be INFO line 0.
    ffr.goToPosition(info1.stt_pos, info1.line_num);
    ffr.getPreviousValidLine(buf, pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(0));
  }

  free(buf);
  free(buf2);
  freeLineFormat(lf);
  teardown();
}

// ──────────────────────────────────────────────
// skipNextRawLines – advances exactly N raw lines
// ──────────────────────────────────────────────

TEST_CASE("FilteredFileReader::skipNextRawLines - advances exactly N raw lines") {
  setup();
  LineFormat* lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log";
  std::string val = "INFO";
  auto filter = std::make_shared<FieldFilter>(lf, "Level", FilterComparison::EQUAL, &val);

  FilteredFileReader ffr(filename, lf, filter, 10);
  ffr.m_accept_bad_format = false;
  char* buf = (char*)malloc(ffr.getMaxCharsPerLine());
  char* buf2 = (char*)malloc(ffr.getMaxCharsPerLine());
  ProcessedLine pl;

  // INFO lines are at raw lines: 4, 12, 14, 20, ...  (0-indexed)

  SECTION("skipNextRawLines(0) does not move the position") {
    ffr.seekRawLine(0);
    ffr.skipNextRawLines(0);
    ffr.getNextValidLine(buf, pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(0));
  }

  SECTION("skipNextRawLines lands exactly on an INFO line") {
    // raw line 4 is an INFO line; skip to it and it is the next result.
    ffr.seekRawLine(0);
    ffr.skipNextRawLines(count_to_info_line(0)); // skip 4 lines → now at raw line 4
    ffr.getNextValidLine(buf, pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(0));
  }

  SECTION("skipNextRawLines lands past an INFO line, next valid is the following one") {
    // raw line 4 is INFO; skip 5 lines → now at raw line 5 (not INFO).
    // Next INFO line is at raw line 12.
    ffr.seekRawLine(0);
    ffr.skipNextRawLines(count_to_info_line(0) + 1);
    ffr.getNextValidLine(buf, pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(1));
  }

  SECTION("skipNextRawLines(1) after goToPosition steps past the seeked line") {
    ProcessedLine info1;
    ffr.getNextValidLine(buf, pl);       // INFO line 0
    ffr.getNextValidLine(buf2, info1);   // INFO line 1 at raw line 12

    // Seek to the start of INFO line 1, then step past it.
    ffr.goToPosition(info1.stt_pos, info1.line_num);
    ffr.skipNextRawLines(1);
    ffr.getNextValidLine(buf, pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(2)); // INFO line 2 at raw line 14
  }

  free(buf);
  free(buf2);
  freeLineFormat(lf);
  teardown();
}
