#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "test_helpers.hpp"
#include "log_parser_terminal.hpp"
#include "ConfigHandler.hpp"

#include <algorithm>
#include <string>

#define SAMPLE_LOG TEST_FOLDER "data/sample.log"

// sample.log has 62 raw lines
static constexpr int TOTAL_LINES = 62;
// With INFO filter + accept_bad_format: 10 INFO + 4 binary = 14
static constexpr int FILTERED_LINES = 14;

static LogParserInterface* make_unfiltered_lpi() {
  std::string filename = SAMPLE_LOG;
  return new LogParserInterface(filename, getDefaultLineFormat(), nullptr);
}

static LogParserInterface* make_info_filtered_lpi() {
  std::string filename = SAMPLE_LOG;
  auto lf = getDefaultLineFormat();
  std::string val = "INFO";
  auto filter = std::make_shared<FieldFilter>(
      lf.get(), "Level", FilterComparison::EQUAL, &val);
  return new LogParserInterface(filename, std::move(lf), filter);
}

// ============================================================
// updateDisplayState — displayed_pls correctness
// ============================================================

TEST_CASE("updateDisplayState - offset 0 no filter") {
  setup();
  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 25;
  term.term_state.ncols = 80;
  term.term_state.line_offset = 0;

  term.updateDisplayState();

  REQUIRE(term.term_state.displayed_pls.size() == 24); // nrows - status_line
  for (int i = 0; i < 24; i++) {
    REQUIRE(term.term_state.displayed_pls[i] != nullptr);
    REQUIRE(term.term_state.displayed_pls[i] == lpi->getLine(i).line);
  }
  teardown();
}

TEST_CASE("updateDisplayState - non-zero offset no filter") {
  setup();
  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.ncols = 80;

  SECTION("offset 5, nrows 15") {
    term.term_state.nrows = 15;
    term.term_state.line_offset = 5;
    term.updateDisplayState();

    REQUIRE(term.term_state.displayed_pls.size() == 14);
    for (int i = 0; i < 14; i++) {
      REQUIRE(term.term_state.displayed_pls[i] == lpi->getLine(i + 5).line);
    }
  }

  SECTION("offset 20, nrows 15") {
    term.term_state.nrows = 15;
    term.term_state.line_offset = 20;
    term.updateDisplayState();

    REQUIRE(term.term_state.displayed_pls.size() == 14);
    for (int i = 0; i < 14; i++) {
      REQUIRE(term.term_state.displayed_pls[i] == lpi->getLine(i + 20).line);
    }
  }

  SECTION("offset 40, nrows 10") {
    term.term_state.nrows = 10;
    term.term_state.line_offset = 40;
    term.updateDisplayState();

    REQUIRE(term.term_state.displayed_pls.size() == 9);
    for (int i = 0; i < 9; i++) {
      REQUIRE(term.term_state.displayed_pls[i] == lpi->getLine(i + 40).line);
    }
  }

  teardown();
}

TEST_CASE("updateDisplayState - INFO filter offset 0") {
  setup();
  auto* lpi = make_info_filtered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 20;
  term.term_state.ncols = 80;
  term.term_state.line_offset = 0;

  term.updateDisplayState();

  REQUIRE(term.term_state.displayed_pls.size() == 19);
  // First 14 entries should match the filtered lines
  for (int i = 0; i < FILTERED_LINES; i++) {
    REQUIRE(term.term_state.displayed_pls[i] != nullptr);
    REQUIRE(term.term_state.displayed_pls[i] == lpi->getLine(i).line);
    REQUIRE(SV_TO_STR(term.term_state.displayed_pls[i]->raw_line) ==
            SV_TO_STR(info_and_bf_lines[i]));
  }
  // Remaining entries should be nullptr (only 14 filtered lines)
  for (int i = FILTERED_LINES; i < 19; i++) {
    REQUIRE(term.term_state.displayed_pls[i] == nullptr);
  }
  teardown();
}

TEST_CASE("updateDisplayState - INFO filter non-zero offset") {
  setup();
  auto* lpi = make_info_filtered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 20;
  term.term_state.ncols = 80;

  SECTION("offset 3") {
    term.term_state.line_offset = 3;
    term.updateDisplayState();

    int remaining = FILTERED_LINES - 3; // 11 lines left
    for (int i = 0; i < remaining; i++) {
      REQUIRE(term.term_state.displayed_pls[i] != nullptr);
      REQUIRE(SV_TO_STR(term.term_state.displayed_pls[i]->raw_line) ==
              SV_TO_STR(info_and_bf_lines[i + 3]));
    }
    for (int i = remaining; i < 19; i++) {
      REQUIRE(term.term_state.displayed_pls[i] == nullptr);
    }
  }

  SECTION("offset 10") {
    term.term_state.line_offset = 10;
    term.updateDisplayState();

    int remaining = FILTERED_LINES - 10; // 4 lines left
    for (int i = 0; i < remaining; i++) {
      REQUIRE(term.term_state.displayed_pls[i] != nullptr);
      REQUIRE(SV_TO_STR(term.term_state.displayed_pls[i]->raw_line) ==
              SV_TO_STR(info_and_bf_lines[i + 10]));
    }
    for (int i = remaining; i < 19; i++) {
      REQUIRE(term.term_state.displayed_pls[i] == nullptr);
    }
  }

  teardown();
}

TEST_CASE("updateDisplayState - offset past end of file no filter") {
  setup();
  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 10;
  term.term_state.ncols = 80;
  term.term_state.line_offset = 100; // well past 62 lines

  term.updateDisplayState();

  REQUIRE(term.term_state.displayed_pls.size() == 9);
  for (int i = 0; i < 9; i++) {
    REQUIRE(term.term_state.displayed_pls[i] == nullptr);
  }
  teardown();
}

TEST_CASE("updateDisplayState - offset past end of filtered file") {
  setup();
  auto* lpi = make_info_filtered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 10;
  term.term_state.ncols = 80;
  term.term_state.line_offset = 50; // well past 14 filtered lines

  term.updateDisplayState();

  REQUIRE(term.term_state.displayed_pls.size() == 9);
  for (int i = 0; i < 9; i++) {
    REQUIRE(term.term_state.displayed_pls[i] == nullptr);
  }
  teardown();
}

TEST_CASE("updateDisplayState - partial page near end of file") {
  setup();
  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 15; // 14 displayed rows
  term.term_state.ncols = 80;
  term.term_state.line_offset = 55; // lines 55-61 exist (7 lines), 62-68 don't

  term.updateDisplayState();

  REQUIRE(term.term_state.displayed_pls.size() == 14);
  int existing = TOTAL_LINES - 55; // 7
  for (int i = 0; i < existing; i++) {
    REQUIRE(term.term_state.displayed_pls[i] != nullptr);
    REQUIRE(term.term_state.displayed_pls[i] == lpi->getLine(i + 55).line);
  }
  for (int i = existing; i < 14; i++) {
    REQUIRE(term.term_state.displayed_pls[i] == nullptr);
  }
  teardown();
}

TEST_CASE("updateDisplayState - partial page near end of filtered file") {
  setup();
  auto* lpi = make_info_filtered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 10; // 9 displayed rows
  term.term_state.ncols = 80;
  term.term_state.line_offset = 10; // filtered lines 10-13 exist (4 lines)

  term.updateDisplayState();

  REQUIRE(term.term_state.displayed_pls.size() == 9);
  int existing = FILTERED_LINES - 10; // 4
  for (int i = 0; i < existing; i++) {
    REQUIRE(term.term_state.displayed_pls[i] != nullptr);
  }
  for (int i = existing; i < 9; i++) {
    REQUIRE(term.term_state.displayed_pls[i] == nullptr);
  }
  teardown();
}

TEST_CASE("updateDisplayState - nrows=2 shows exactly one line") {
  setup();
  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 2; // 1 status line + 1 content line
  term.term_state.ncols = 80;
  term.term_state.line_offset = 0;

  term.updateDisplayState();

  REQUIRE(term.term_state.displayed_pls.size() == 1);
  REQUIRE(term.term_state.displayed_pls[0] != nullptr);
  REQUIRE(term.term_state.displayed_pls[0] == lpi->getLine(0).line);
  teardown();
}

TEST_CASE("updateDisplayState - nrows=1 shows no content lines") {
  setup();
  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 1; // only status line
  term.term_state.ncols = 80;
  term.term_state.line_offset = 0;

  term.updateDisplayState();

  REQUIRE(term.term_state.displayed_pls.empty());
  teardown();
}

// ============================================================
// updateDisplayState — info_col_size
// ============================================================

TEST_CASE("updateDisplayState - info_col_size matches line numbers") {
  setup();
  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.ncols = 80;

  SECTION("small line numbers") {
    term.term_state.nrows = 6; // 5 displayed rows, lines 0-4
    term.term_state.line_offset = 0;
    term.updateDisplayState();

    // Last line_num is 4, "4" has 1 digit => info_col_size = 2 + 1 = 3
    REQUIRE(term.term_state.info_col_size == 3);
  }

  SECTION("two-digit line numbers") {
    term.term_state.nrows = 25; // 24 displayed rows, lines 0-23
    term.term_state.line_offset = 0;
    term.updateDisplayState();

    // Last line_num is 23, "23" has 2 digits => info_col_size = 2 + 2 = 4
    REQUIRE(term.term_state.info_col_size == 4);
  }

  SECTION("with offset to higher line numbers") {
    term.term_state.nrows = 5;
    term.term_state.line_offset = 55;
    term.updateDisplayState();

    // Last existing line is 61, "61" has 2 digits => info_col_size = 2 + 2 = 4
    REQUIRE(term.term_state.info_col_size == 4);
  }

  teardown();
}

TEST_CASE("updateDisplayState - info_col_size with filter") {
  setup();
  auto* lpi = make_info_filtered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 20;
  term.term_state.ncols = 80;
  term.term_state.line_offset = 0;

  term.updateDisplayState();

  // Last filtered line (local 13) has global line_num 57
  // "57" has 2 digits => info_col_size = 2 + 2 = 4
  REQUIRE(term.term_state.info_col_size == 4);
  teardown();
}

TEST_CASE("updateDisplayState - info_col_size with local line numbers") {
  setup();
  ConfigHandler cfg;
  std::string orig = cfg.get(CFG_COMMON_PROFILE, CFG_LINE_NUM_MODE);
  cfg.set(CFG_COMMON_PROFILE, CFG_LINE_NUM_MODE, "local");

  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 25;
  term.term_state.ncols = 80;
  term.term_state.line_offset = 0;

  term.updateDisplayState();

  // local mode: info_col_size = 2 + digits_of(nrows - num_status_line)
  // = 2 + digits_of(25 - 1) = 2 + digits_of(24) = 2 + 2 = 4
  REQUIRE(term.term_state.info_col_size == 4);

  cfg.set(CFG_COMMON_PROFILE, CFG_LINE_NUM_MODE, orig);
  teardown();
}

// ============================================================
// drawRows — frame content
// ============================================================

TEST_CASE("drawRows - frame contains text of displayed lines") {
  setup();
  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 10;
  term.term_state.ncols = 120;
  term.term_state.line_offset = 0;

  term.updateDisplayState();
  term.frame_str = "";
  term.drawRows();

  // Each displayed line's raw text should appear in the frame
  for (int i = 0; i < 9; i++) {
    const ProcessedLine* pl = term.term_state.displayed_pls[i];
    REQUIRE(pl != nullptr);
    std::string text(pl->raw_line);
    REQUIRE(term.frame_str.find(text) != std::string::npos);
  }
  teardown();
}

TEST_CASE("drawRows - offset changes visible line text") {
  setup();
  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 5;
  term.term_state.ncols = 120;

  // Capture frame at offset 0
  term.term_state.line_offset = 0;
  term.updateDisplayState();
  term.frame_str = "";
  term.drawRows();
  std::string frame_at_0 = term.frame_str;

  // Capture frame at offset 30
  term.term_state.line_offset = 30;
  term.updateDisplayState();
  term.frame_str = "";
  term.drawRows();
  std::string frame_at_30 = term.frame_str;

  // Line at offset 0 should be in frame_at_0 but not frame_at_30
  std::string first_line(lpi->getLine(0).line->raw_line);
  REQUIRE(frame_at_0.find(first_line) != std::string::npos);
  REQUIRE(frame_at_30.find(first_line) == std::string::npos);

  // Line at offset 30 should be in frame_at_30 but not frame_at_0
  std::string line_30(lpi->getLine(30).line->raw_line);
  REQUIRE(frame_at_30.find(line_30) != std::string::npos);
  REQUIRE(frame_at_0.find(line_30) == std::string::npos);

  teardown();
}

TEST_CASE("drawRows - INFO filter: only filtered lines appear") {
  setup();
  auto* lpi = make_info_filtered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 20;
  term.term_state.ncols = 120;
  term.term_state.line_offset = 0;

  term.updateDisplayState();
  term.frame_str = "";
  term.drawRows();

  // All info_and_bf_lines should appear
  for (int i = 0; i < FILTERED_LINES; i++) {
    std::string text(info_and_bf_lines[i]);
    REQUIRE(term.frame_str.find(text) != std::string::npos);
  }

  // A TRACE-only line from the file should NOT appear
  REQUIRE(term.frame_str.find("TRACE") == std::string::npos);
  teardown();
}

TEST_CASE("drawRows - line numbers appear with tilde separator") {
  setup();
  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 6;
  term.term_state.ncols = 120;
  term.term_state.line_offset = 0;

  term.updateDisplayState();
  term.frame_str = "";
  term.drawRows();

  // Each displayed line should have its line number followed by "~ "
  for (int i = 0; i < 5; i++) {
    const ProcessedLine* pl = term.term_state.displayed_pls[i];
    REQUIRE(pl != nullptr);
    std::string marker = std::to_string(pl->line_num) + "~ ";
    REQUIRE(term.frame_str.find(marker) != std::string::npos);
  }
  teardown();
}

TEST_CASE("drawRows - local line numbers are sequential") {
  setup();
  ConfigHandler cfg;
  std::string orig = cfg.get(CFG_COMMON_PROFILE, CFG_LINE_NUM_MODE);
  cfg.set(CFG_COMMON_PROFILE, CFG_LINE_NUM_MODE, "local");

  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 6;
  term.term_state.ncols = 120;
  term.term_state.line_offset = 10;

  term.updateDisplayState();
  term.frame_str = "";
  term.drawRows();

  // With local numbering and offset=10, lines should show their
  // local IDs (1-indexed position among filtered lines): 11, 12, 13, 14, 15
  for (int i = 0; i < 5; i++) {
    std::string marker = std::to_string(i + 11) + "~ ";
    REQUIRE(term.frame_str.find(marker) != std::string::npos);
  }

  cfg.set(CFG_COMMON_PROFILE, CFG_LINE_NUM_MODE, orig);
  teardown();
}

TEST_CASE("drawRows - empty rows past end of file") {
  setup();
  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 10;
  term.term_state.ncols = 80;
  term.term_state.line_offset = 100; // past end

  term.updateDisplayState();
  term.frame_str = "";
  term.drawRows();

  // Frame should not contain any actual log content
  REQUIRE(term.frame_str.find("085338") == std::string::npos);
  REQUIRE(term.frame_str.find("TRACE") == std::string::npos);
  REQUIRE(term.frame_str.find("INFO") == std::string::npos);
  // The "~ " line number marker should not appear (no lines to number)
  REQUIRE(term.frame_str.find("~ ") == std::string::npos);
  teardown();
}

TEST_CASE("drawRows - lines truncated at narrow ncols") {
  setup();
  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 3;
  term.term_state.ncols = 30; // very narrow
  term.term_state.line_offset = 0;

  term.updateDisplayState();
  term.frame_str = "";
  term.drawRows();

  // The first raw line is ~76 chars. With ncols=30 and info_col_size ~3,
  // only ~27 chars of the line should appear. The full line should NOT
  // appear in the frame.
  const ProcessedLine* pl = term.term_state.displayed_pls[0];
  REQUIRE(pl != nullptr);
  std::string full_line(pl->raw_line);
  REQUIRE(full_line.size() > 30);
  REQUIRE(term.frame_str.find(full_line) == std::string::npos);

  // But a prefix of the line should appear
  std::string prefix = full_line.substr(0, 20);
  REQUIRE(term.frame_str.find(prefix) != std::string::npos);
  teardown();
}

TEST_CASE("drawRows - hide_bad_fmt hides malformed lines") {
  setup();
  ConfigHandler cfg;
  std::string orig = cfg.get(CFG_COMMON_PROFILE, CFG_HIDE_BAD_FMT);
  cfg.set(CFG_COMMON_PROFILE, CFG_HIDE_BAD_FMT, "true");

  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  // Binary lines are at global positions 25-28 (0-indexed)
  term.term_state.nrows = 10;
  term.term_state.ncols = 120;
  term.term_state.line_offset = 24; // show lines 24-32, includes binary at 25-28

  term.updateDisplayState();
  term.frame_str = "";
  term.drawRows();

  // Binary line content should NOT appear when hide_bad_fmt is true
  REQUIRE(term.frame_str.find("0x00 0x01 0x02 0x03") == std::string::npos);
  REQUIRE(term.frame_str.find("0x04 0x05 0x06 0x07") == std::string::npos);

  // But well-formatted lines in the same range should appear
  // Line 24 is a TRACE line (well formatted)
  const ProcessedLine* pl24 = term.term_state.displayed_pls[0];
  REQUIRE(pl24 != nullptr);
  if (pl24->well_formated) {
    std::string text(pl24->raw_line);
    REQUIRE(term.frame_str.find(text) != std::string::npos);
  }

  cfg.set(CFG_COMMON_PROFILE, CFG_HIDE_BAD_FMT, orig);
  teardown();
}

TEST_CASE("drawRows - bad format lines visible when not hidden") {
  setup();
  ConfigHandler cfg;
  std::string orig = cfg.get(CFG_COMMON_PROFILE, CFG_HIDE_BAD_FMT);
  cfg.set(CFG_COMMON_PROFILE, CFG_HIDE_BAD_FMT, "false");

  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 10;
  term.term_state.ncols = 120;
  term.term_state.line_offset = 25; // starts at first binary line

  term.updateDisplayState();
  term.frame_str = "";
  term.drawRows();

  // Binary lines should appear when hide_bad_fmt is false
  REQUIRE(term.frame_str.find("0x00 0x01 0x02 0x03") != std::string::npos);
  REQUIRE(term.frame_str.find("0x04 0x05 0x06 0x07") != std::string::npos);

  cfg.set(CFG_COMMON_PROFILE, CFG_HIDE_BAD_FMT, orig);
  teardown();
}

TEST_CASE("drawRows - status line reflects input mode") {
  setup();
  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 5;
  term.term_state.ncols = 120;
  term.term_state.line_offset = 0;

  SECTION("ACTION mode shows Status line") {
    term.term_state.input_mode = ACTION;
    term.updateDisplayState();
    term.frame_str = "";
    term.drawRows();
    REQUIRE(term.frame_str.find("Status:") != std::string::npos);
  }

  SECTION("RAW mode shows raw input") {
    term.term_state.input_mode = RAW;
    term.term_state.raw_input = ":search_text";
    term.updateDisplayState();
    term.frame_str = "";
    term.drawRows();
    REQUIRE(term.frame_str.find(":search_text") != std::string::npos);
  }

  teardown();
}

TEST_CASE("drawRows - full page vs partial page line count") {
  setup();
  auto* lpi = make_info_filtered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 20;
  term.term_state.ncols = 120;
  term.term_state.line_offset = 0;

  term.updateDisplayState();
  term.frame_str = "";
  term.drawRows();

  // Count how many "~ " markers appear (one per rendered line)
  size_t count = 0;
  size_t pos = 0;
  while ((pos = term.frame_str.find("~ ", pos)) != std::string::npos) {
    count++;
    pos += 2;
  }

  // Should be exactly FILTERED_LINES (14) line markers, not 19 (nrows-1)
  REQUIRE(count == FILTERED_LINES);
  teardown();
}

TEST_CASE("drawRows - consistent between repeated calls") {
  setup();
  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 10;
  term.term_state.ncols = 100;
  term.term_state.line_offset = 5;

  term.updateDisplayState();
  term.frame_str = "";
  term.drawRows();
  std::string first_frame = term.frame_str;

  // Call again with same state
  term.updateDisplayState();
  term.frame_str = "";
  term.drawRows();
  std::string second_frame = term.frame_str;

  REQUIRE(first_frame == second_frame);
  teardown();
}

TEST_CASE("drawRows - long raw input not truncated") {
  setup();
  auto* lpi = make_unfiltered_lpi();
  LogParserTerminal term(lpi);
  term.term_state.nrows = 5;
  term.term_state.ncols = 200;
  term.term_state.line_offset = 0;
  term.term_state.input_mode = RAW;

  // 82 characters — exceeds the snprintf(buf, 80, ...) limit in drawRows
  std::string long_input = ":filter Level == INFO AND Source CONTAINS router_forward_getOI AND Time > 085400";
  REQUIRE(long_input.size() > 79);
  term.term_state.raw_input = long_input;
  term.term_state.raw_input_cursor = long_input.size();

  term.updateDisplayState();
  term.frame_str = "";
  term.drawRows();

  // The tail of the input must not be silently dropped
  REQUIRE(term.frame_str.find(long_input) != std::string::npos);
  teardown();
}
