#include <catch2/catch_test_macros.hpp>
#include "test_helpers.hpp"


// ============================================================
// Unfiltered — forward search
// ============================================================

TEST_CASE("findNextOccurence - forward finds first match") {
  setup();
  auto* lpi = make_lpi();

  // "LASTLINE" only appears on global line 61
  auto [line, pos] = lpi->findNextOccurence("LASTLINE", 0);
  REQUIRE(line == 61);
  REQUIRE(pos != SIZE_MAX);

  teardown();
}

TEST_CASE("findNextOccurence - forward returns correct position") {
  setup();
  auto* lpi = make_lpi();

  // "Ioctl" first appears on global line 20
  auto [line, pos] = lpi->findNextOccurence("Ioctl", 0);
  REQUIRE(line == 20);

  // Verify pos by reading the line and checking find() agrees
  std::string raw(lpi->getLine(20).line->raw_line);
  REQUIRE(pos == raw.find("Ioctl"));

  teardown();
}

TEST_CASE("findNextOccurence - forward skips earlier lines") {
  setup();
  auto* lpi = make_lpi();

  // "constructing a PATH" appears at global lines 5, 30, 42
  // Searching from line 6 should skip line 5 and find line 30
  auto [line, pos] = lpi->findNextOccurence("constructing a PATH", 6);
  REQUIRE(line == 30);

  teardown();
}

TEST_CASE("findNextOccurence - forward no match returns sentinel") {
  setup();
  auto* lpi = make_lpi();

  auto [line, pos] = lpi->findNextOccurence("NONEXISTENT_STRING", 0);
  REQUIRE(line == LINE_T_MAX);
  REQUIRE(pos == SIZE_MAX);

  teardown();
}

// ============================================================
// Unfiltered — backward search
// ============================================================

TEST_CASE("findNextOccurence - backward finds last match before from") {
  setup();
  auto* lpi = make_lpi();

  // "constructing a PATH" at global 5, 30, 42
  // Backward from 61 should find 42
  auto [line, pos] = lpi->findNextOccurence("constructing a PATH", 61, false);
  REQUIRE(line == 42);

  teardown();
}

TEST_CASE("findNextOccurence - backward from middle") {
  setup();
  auto* lpi = make_lpi();

  // "constructing a PATH" at global 5, 30, 42
  // Backward from 35 should find 30
  auto [line, pos] = lpi->findNextOccurence("constructing a PATH", 35, false);
  REQUIRE(line == 30);

  teardown();
}

TEST_CASE("findNextOccurence - backward no match returns sentinel") {
  setup();
  auto* lpi = make_lpi();

  // "constructing a PATH" first appears at global 5
  // Backward from 4 should find nothing
  auto [line, pos] = lpi->findNextOccurence("constructing a PATH", 4, false);
  REQUIRE(line == LINE_T_MAX);
  REQUIRE(pos == SIZE_MAX);

  teardown();
}

// ============================================================
// Unfiltered — small block size (same logic, exercises block traversal)
// ============================================================

TEST_CASE("findNextOccurence - forward with small block size") {
  setup();
  auto* lpi = make_lpi(10);

  auto [line, pos] = lpi->findNextOccurence("LASTLINE", 0);
  REQUIRE(line == 61);
  REQUIRE(pos != SIZE_MAX);

  teardown();
}

// ============================================================
// Filtered — forward search
// ============================================================

TEST_CASE("findNextOccurence - filtered: finds match in INFO line") {
  setup();
  auto* lpi = make_info_filtered_lpi();

  // "Ioctl" appears in INFO lines at global 20, 36, 57
  auto [line, pos] = lpi->findNextOccurence("Ioctl", 0);
  REQUIRE(line == 20);
  REQUIRE(pos != SIZE_MAX);

  teardown();
}

TEST_CASE("findNextOccurence - filtered: skips to next INFO match") {
  setup();
  auto* lpi = make_info_filtered_lpi();

  // From global line 21 (past first Ioctl), next should be global 36
  auto [line, pos] = lpi->findNextOccurence("Ioctl", 21);
  REQUIRE(line == 36);

  teardown();
}

TEST_CASE("findNextOccurence - filtered: TRACE-only string not found") {
  setup();
  auto* lpi = make_info_filtered_lpi();

  // "constructing a PATH" only appears in TRACE lines (global 5, 30, 42)
  // With INFO filter, these lines are skipped entirely
  auto [line, pos] = lpi->findNextOccurence("constructing a PATH", 0);
  REQUIRE(line == LINE_T_MAX);
  REQUIRE(pos == SIZE_MAX);

  teardown();
}

TEST_CASE("findNextOccurence - filtered: EVENT-only string not found") {
  setup();
  auto* lpi = make_info_filtered_lpi();

  // "SIGALRM" only appears in EVENT lines (global 18, 34, 55)
  auto [line, pos] = lpi->findNextOccurence("SIGALRM", 0);
  REQUIRE(line == LINE_T_MAX);
  REQUIRE(pos == SIZE_MAX);

  teardown();
}

TEST_CASE("findNextOccurence - filtered: string in both INFO and TRACE only matches INFO") {
  setup();
  auto* lpi = make_info_filtered_lpi();

  // "rsvp_flow_stateMachine" appears in both TRACE and INFO lines
  // First TRACE: global 7 ("reentering state RESVED")
  // First INFO: global 4 ("state RESVED, event T1OUT")
  // Search should find global 4 (the INFO line), not 7
  auto [line, pos] = lpi->findNextOccurence("rsvp_flow_stateMachine", 0);
  REQUIRE(line == 4);

  teardown();
}

// ============================================================
// Filtered — backward search
// ============================================================

TEST_CASE("findNextOccurence - filtered: backward finds last INFO match") {
  setup();
  auto* lpi = make_info_filtered_lpi();

  // "Ioctl" in INFO lines at global 20, 36, 57
  // Backward from global 61 should find 57
  auto [line, pos] = lpi->findNextOccurence("Ioctl", 61, false);
  REQUIRE(line == 57);

  teardown();
}

TEST_CASE("findNextOccurence - filtered: backward skips TRACE lines") {
  setup();
  auto* lpi = make_info_filtered_lpi();

  // "rsvp_flow_stateMachine" in TRACE at global 7, 17, 32, 44, 54
  //                          in INFO at global 4, 14, 29, 41, 51
  // Backward from 50 should find global 41 (INFO), not 44 (TRACE)
  auto [line, pos] = lpi->findNextOccurence("rsvp_flow_stateMachine", 50, false);
  REQUIRE(line == 41);

  teardown();
}
