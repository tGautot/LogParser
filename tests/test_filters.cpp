#include <catch2/catch_test_macros.hpp>

#include "line_format.hpp"
#include "line_filter.hpp"
#include "line_parser.hpp"
#include "parsing_data.hpp"
#include "processed_line.hpp"

#include <memory>
#include <string>

// ──────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────

// Format: {INT:Val} {STR:Name}
// Sample line: "100 hello"  →  Val=100, Name="hello"
static std::unique_ptr<LineFormat> makeSimpleFormat() {
  return LineFormat::fromFormatString("{INT:Val} {STR:Name}");
}

// NOTE: RawLineFilter and LineNumberFilter are not tested here because
// both classes have private constructors and cannot be instantiated
// from outside the class. A factory method or friend declaration would
// be needed to enable unit-testing them directly.

// ──────────────────────────────────────────────
// FieldFilter – int comparisons
// ──────────────────────────────────────────────

TEST_CASE("FieldFilter - int comparisons") {
  std::unique_ptr<LineFormat> lf = makeSimpleFormat();
  std::shared_ptr<Parser> parser  = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(parser->format.get());
  std::string line = "100 hello";
  REQUIRE(parser->parseLine(line, &pl));

  SECTION("EQUAL") {
    int64_t v = 100;
    FieldFilter hit(parser->format.get(), "Val", FilterComparison::EQUAL, &v);
    REQUIRE(hit.passes(&pl));

    v = 99;
    FieldFilter miss(parser->format.get(), "Val", FilterComparison::EQUAL, &v);
    REQUIRE_FALSE(miss.passes(&pl));
  }

  SECTION("SMALLER") {
    int64_t v = 101;
    FieldFilter hit(parser->format.get(), "Val", FilterComparison::SMALLER, &v);
    REQUIRE(hit.passes(&pl)); // 100 < 101

    v = 100;
    FieldFilter miss(parser->format.get(), "Val", FilterComparison::SMALLER, &v);
    REQUIRE_FALSE(miss.passes(&pl)); // 100 < 100 → false
  }

  SECTION("SMALLER_EQ") {
    int64_t v = 100;
    FieldFilter hit(parser->format.get(), "Val", FilterComparison::SMALLER_EQ, &v);
    REQUIRE(hit.passes(&pl)); // 100 <= 100

    v = 99;
    FieldFilter miss(parser->format.get(), "Val", FilterComparison::SMALLER_EQ, &v);
    REQUIRE_FALSE(miss.passes(&pl)); // 100 <= 99 → false
  }

  SECTION("GREATER") {
    int64_t v = 99;
    FieldFilter hit(parser->format.get(), "Val", FilterComparison::GREATER, &v);
    REQUIRE(hit.passes(&pl)); // 100 > 99

    v = 100;
    FieldFilter miss(parser->format.get(), "Val", FilterComparison::GREATER, &v);
    REQUIRE_FALSE(miss.passes(&pl)); // 100 > 100 → false
  }

  SECTION("GREATER_EQ") {
    int64_t v = 100;
    FieldFilter hit(parser->format.get(), "Val", FilterComparison::GREATER_EQ, &v);
    REQUIRE(hit.passes(&pl)); // 100 >= 100

    v = 101;
    FieldFilter miss(parser->format.get(), "Val", FilterComparison::GREATER_EQ, &v);
    REQUIRE_FALSE(miss.passes(&pl)); // 100 >= 101 → false
  }

}

// ──────────────────────────────────────────────
// FieldFilter – string comparisons
// ──────────────────────────────────────────────

TEST_CASE("FieldFilter - string comparisons") {
  std::unique_ptr<LineFormat> lf  = makeSimpleFormat();
  std::shared_ptr<Parser> parser  = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(parser->format.get());
  std::string line = "100 hello";
  REQUIRE(parser->parseLine(line, &pl));

  SECTION("EQUAL") {
    std::string v = "hello";
    FieldFilter hit(parser->format.get(), "Name", FilterComparison::EQUAL, v);
    REQUIRE(hit.passes(&pl));

    std::string v2 = "world";
    FieldFilter miss(parser->format.get(), "Name", FilterComparison::EQUAL, v2);
    REQUIRE_FALSE(miss.passes(&pl));
  }

  SECTION("SMALLER - lexicographic") {
    std::string v = "world"; // "hello" < "world"
    FieldFilter hit(parser->format.get(), "Name", FilterComparison::SMALLER, v);
    REQUIRE(hit.passes(&pl));

    std::string v2 = "apple"; // "hello" < "apple" → false
    FieldFilter miss(parser->format.get(), "Name", FilterComparison::SMALLER, v2);
    REQUIRE_FALSE(miss.passes(&pl));
  }

  SECTION("SMALLER_EQ") {
    std::string v = "hello"; // "hello" <= "hello"
    FieldFilter hit(parser->format.get(), "Name", FilterComparison::SMALLER_EQ, v);
    REQUIRE(hit.passes(&pl));

    std::string v2 = "apple"; // "hello" <= "apple" → false
    FieldFilter miss(parser->format.get(), "Name", FilterComparison::SMALLER_EQ, v2);
    REQUIRE_FALSE(miss.passes(&pl));
  }

  SECTION("GREATER - lexicographic") {
    std::string v = "apple"; // "hello" > "apple"
    FieldFilter hit(parser->format.get(), "Name", FilterComparison::GREATER, v);
    REQUIRE(hit.passes(&pl));

    std::string v2 = "world"; // "hello" > "world" → false
    FieldFilter miss(parser->format.get(), "Name", FilterComparison::GREATER, v2);
    REQUIRE_FALSE(miss.passes(&pl));
  }

  SECTION("GREATER_EQ") {
    std::string v = "hello"; // "hello" >= "hello"
    FieldFilter hit(parser->format.get(), "Name", FilterComparison::GREATER_EQ, v);
    REQUIRE(hit.passes(&pl));

    std::string v2 = "zoo"; // "hello" >= "zoo" → false
    FieldFilter miss(parser->format.get(), "Name", FilterComparison::GREATER_EQ, v2);
    REQUIRE_FALSE(miss.passes(&pl));
  }

  SECTION("CONTAINS") {
    std::string v = "ell"; // "hello" contains "ell"
    FieldFilter hit(parser->format.get(), "Name", FilterComparison::CONTAINS, v);
    REQUIRE(hit.passes(&pl));

    std::string v2 = "world";
    FieldFilter miss(parser->format.get(), "Name", FilterComparison::CONTAINS, v2);
    REQUIRE_FALSE(miss.passes(&pl));

    std::string v3 = "hello"; // exact match also counts as CONTAINS
    FieldFilter exact(parser->format.get(), "Name", FilterComparison::CONTAINS, v3);
    REQUIRE(exact.passes(&pl));
  }

  SECTION("BEGINS_WITH") {
    std::string v = "hel";
    FieldFilter hit(parser->format.get(), "Name", FilterComparison::BEGINS_WITH, v);
    REQUIRE(hit.passes(&pl));

    std::string v2 = "ello";
    FieldFilter miss(parser->format.get(), "Name", FilterComparison::BEGINS_WITH, v2);
    REQUIRE_FALSE(miss.passes(&pl));
  }

  SECTION("ENDS_WITH") {
    std::string v = "llo";
    FieldFilter hit(parser->format.get(), "Name", FilterComparison::ENDS_WITH, v);
    REQUIRE(hit.passes(&pl));

    std::string v2 = "hel";
    FieldFilter miss(parser->format.get(), "Name", FilterComparison::ENDS_WITH, v2);
    REQUIRE_FALSE(miss.passes(&pl));
  }

  SECTION("CASE_INS_CONTAINS") {
    std::string v = "ELL"; // "hello" contains "ell" case-insensitively
    FieldFilter hit(parser->format.get(), "Name", FilterComparison::CASE_INS_CONTAINS, v);
    REQUIRE(hit.passes(&pl));

    std::string v2 = "HeLLo"; // mixed case full match
    FieldFilter mixed(parser->format.get(), "Name", FilterComparison::CASE_INS_CONTAINS, v2);
    REQUIRE(mixed.passes(&pl));

    std::string v3 = "WORLD";
    FieldFilter miss(parser->format.get(), "Name", FilterComparison::CASE_INS_CONTAINS, v3);
    REQUIRE_FALSE(miss.passes(&pl));
  }

  SECTION("CASE_INS_BEGINS") {
    std::string v = "HEL"; // "hello" begins with "hel" case-insensitively
    FieldFilter hit(parser->format.get(), "Name", FilterComparison::CASE_INS_BEGINS, v);
    REQUIRE(hit.passes(&pl));

    std::string v2 = "ELLO"; // not a prefix
    FieldFilter miss(parser->format.get(), "Name", FilterComparison::CASE_INS_BEGINS, v2);
    REQUIRE_FALSE(miss.passes(&pl));
  }

  SECTION("CASE_INS_ENDS") {
    std::string v = "LLO"; // "hello" ends with "llo" case-insensitively
    FieldFilter hit(parser->format.get(), "Name", FilterComparison::CASE_INS_ENDS, v);
    REQUIRE(hit.passes(&pl));

    std::string v2 = "HEL"; // not a suffix
    FieldFilter miss(parser->format.get(), "Name", FilterComparison::CASE_INS_ENDS, v2);
    REQUIRE_FALSE(miss.passes(&pl));
  }

}

// ──────────────────────────────────────────────
// FieldFilter – double comparisons
// ──────────────────────────────────────────────

TEST_CASE("FieldFilter - double comparisons") {
  // Format: {DBL:Score}  →  dbl_fields[0]
  std::unique_ptr<LineFormat> lf = LineFormat::fromFormatString("{DBL:Score}");
  std::shared_ptr<Parser> parser = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(parser->format.get());
  std::string line = "3.14";
  REQUIRE(parser->parseLine(line, &pl));

  SECTION("EQUAL") {
    double v = 3.14;
    FieldFilter hit(parser->format.get(), "Score", FilterComparison::EQUAL, &v);
    REQUIRE(hit.passes(&pl));

    double v2 = 2.71;
    FieldFilter miss(parser->format.get(), "Score", FilterComparison::EQUAL, &v2);
    REQUIRE_FALSE(miss.passes(&pl));
  }

  SECTION("SMALLER") {
    double v = 4.0;
    FieldFilter hit(parser->format.get(), "Score", FilterComparison::SMALLER, &v);
    REQUIRE(hit.passes(&pl)); // 3.14 < 4.0

    double v2 = 3.14;
    FieldFilter miss(parser->format.get(), "Score", FilterComparison::SMALLER, &v2);
    REQUIRE_FALSE(miss.passes(&pl)); // 3.14 < 3.14 → false
  }

  SECTION("SMALLER_EQ") {
    double v = 3.14;
    FieldFilter hit(parser->format.get(), "Score", FilterComparison::SMALLER_EQ, &v);
    REQUIRE(hit.passes(&pl)); // 3.14 <= 3.14

    double v2 = 3.0;
    FieldFilter miss(parser->format.get(), "Score", FilterComparison::SMALLER_EQ, &v2);
    REQUIRE_FALSE(miss.passes(&pl)); // 3.14 <= 3.0 → false
  }

  SECTION("GREATER") {
    double v = 3.0;
    FieldFilter hit(parser->format.get(), "Score", FilterComparison::GREATER, &v);
    REQUIRE(hit.passes(&pl)); // 3.14 > 3.0

    double v2 = 3.14;
    FieldFilter miss(parser->format.get(), "Score", FilterComparison::GREATER, &v2);
    REQUIRE_FALSE(miss.passes(&pl)); // 3.14 > 3.14 → false
  }

  SECTION("GREATER_EQ") {
    double v = 3.14;
    FieldFilter hit(parser->format.get(), "Score", FilterComparison::GREATER_EQ, &v);
    REQUIRE(hit.passes(&pl)); // 3.14 >= 3.14

    double v2 = 4.0;
    FieldFilter miss(parser->format.get(), "Score", FilterComparison::GREATER_EQ, &v2);
    REQUIRE_FALSE(miss.passes(&pl)); // 3.14 >= 4.0 → false
  }

}

// ──────────────────────────────────────────────
// LineFilter – inversion
// ──────────────────────────────────────────────

TEST_CASE("LineFilter - invert()") {
  std::unique_ptr<LineFormat> lf  = makeSimpleFormat();
  std::shared_ptr<Parser> parser  = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(parser->format.get());
  std::string line = "100 hello";
  REQUIRE(parser->parseLine(line, &pl));

  std::string val = "hello";
  FieldFilter f(parser->format.get(), "Name", FilterComparison::EQUAL, val);

  REQUIRE(f.passes(&pl));
  REQUIRE_FALSE(f.is_inverted());

  f.invert();
  REQUIRE(f.is_inverted());
  REQUIRE_FALSE(f.passes(&pl)); // same data, now inverted

  f.invert(); // back to normal
  REQUIRE_FALSE(f.is_inverted());
  REQUIRE(f.passes(&pl));

}

// ──────────────────────────────────────────────
// CombinedFilter – logical operators
// ──────────────────────────────────────────────

TEST_CASE("CombinedFilter - all operators") {
  std::unique_ptr<LineFormat> lf  = makeSimpleFormat();
  std::shared_ptr<Parser> parser  = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(parser->format.get());
  std::string line = "100 hello";
  REQUIRE(parser->parseLine(line, &pl));

  // f_pass: Val==100  → passes
  // f_fail: Val==999  → fails
  int64_t pass_val = 100, fail_val = 999;
  auto f_pass = std::make_shared<FieldFilter>(parser->format.get(), "Val", FilterComparison::EQUAL, &pass_val);
  auto f_fail = std::make_shared<FieldFilter>(parser->format.get(), "Val", FilterComparison::EQUAL, &fail_val);

  SECTION("AND: true  AND true  → true") {
    CombinedFilter f(f_pass, f_pass, BitwiseOp::AND);
    REQUIRE(f.passes(&pl));
  }
  SECTION("AND: true  AND false → false") {
    CombinedFilter f(f_pass, f_fail, BitwiseOp::AND);
    REQUIRE_FALSE(f.passes(&pl));
  }
  SECTION("AND: false AND true  → false") {
    CombinedFilter f(f_fail, f_pass, BitwiseOp::AND);
    REQUIRE_FALSE(f.passes(&pl));
  }
  SECTION("AND: false AND false → false") {
    CombinedFilter f(f_fail, f_fail, BitwiseOp::AND);
    REQUIRE_FALSE(f.passes(&pl));
  }

  SECTION("OR: true  OR true   → true") {
    CombinedFilter f(f_pass, f_pass, BitwiseOp::OR);
    REQUIRE(f.passes(&pl));
  }
  SECTION("OR: true  OR false  → true") {
    CombinedFilter f(f_pass, f_fail, BitwiseOp::OR);
    REQUIRE(f.passes(&pl));
  }
  SECTION("OR: false OR true   → true") {
    CombinedFilter f(f_fail, f_pass, BitwiseOp::OR);
    REQUIRE(f.passes(&pl));
  }
  SECTION("OR: false OR false  → false") {
    CombinedFilter f(f_fail, f_fail, BitwiseOp::OR);
    REQUIRE_FALSE(f.passes(&pl));
  }

  SECTION("XOR: true  XOR false → true") {
    CombinedFilter f(f_pass, f_fail, BitwiseOp::XOR);
    REQUIRE(f.passes(&pl));
  }
  SECTION("XOR: false XOR true  → true") {
    CombinedFilter f(f_fail, f_pass, BitwiseOp::XOR);
    REQUIRE(f.passes(&pl));
  }
  SECTION("XOR: true  XOR true  → false") {
    CombinedFilter f(f_pass, f_pass, BitwiseOp::XOR);
    REQUIRE_FALSE(f.passes(&pl));
  }
  SECTION("XOR: false XOR false → false") {
    CombinedFilter f(f_fail, f_fail, BitwiseOp::XOR);
    REQUIRE_FALSE(f.passes(&pl));
  }

  SECTION("NOR: true  NOR true  → false") {
    CombinedFilter f(f_pass, f_pass, BitwiseOp::NOR);
    REQUIRE_FALSE(f.passes(&pl));
  }
  SECTION("NOR: true  NOR false → false") {
    CombinedFilter f(f_pass, f_fail, BitwiseOp::NOR);
    REQUIRE_FALSE(f.passes(&pl));
  }
  SECTION("NOR: false NOR true  → false") {
    CombinedFilter f(f_fail, f_pass, BitwiseOp::NOR);
    REQUIRE_FALSE(f.passes(&pl));
  }
  SECTION("NOR: false NOR false → true") {
    CombinedFilter f(f_fail, f_fail, BitwiseOp::NOR);
    REQUIRE(f.passes(&pl));
  }

}

TEST_CASE("CombinedFilter - inverted combined filter") {
  std::unique_ptr<LineFormat> lf  = makeSimpleFormat();
  std::shared_ptr<Parser> parser  = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(parser->format.get());
  std::string line = "100 hello";
  REQUIRE(parser->parseLine(line, &pl));

  int64_t pass_val = 100, fail_val = 999;
  auto f_pass = std::make_shared<FieldFilter>(parser->format.get(), "Val", FilterComparison::EQUAL, &pass_val);
  auto f_fail = std::make_shared<FieldFilter>(parser->format.get(), "Val", FilterComparison::EQUAL, &fail_val);

  CombinedFilter f(f_pass, f_fail, BitwiseOp::AND); // false
  REQUIRE_FALSE(f.passes(&pl));
  f.invert();
  REQUIRE(f.passes(&pl)); // inverted → true

}

TEST_CASE("CombinedFilter - FieldFilter unknown field throws") {
  std::unique_ptr<LineFormat> lf = makeSimpleFormat();
  REQUIRE_THROWS_AS(
    FieldFilter(lf.get(), "DoesNotExist", FilterComparison::EQUAL, (void*)nullptr),
    std::invalid_argument
  );
}

// ──────────────────────────────────────────────
// RawLineFilter
// ──────────────────────────────────────────────

TEST_CASE("RawLineFilter - substring matching on raw line") {
  // RawLineFilter works on ProcessedLine (raw_line field), not on ParsedLine.
  // We construct a ProcessedLine directly and set its fields.
  std::string content = "0322 085338 INFO rsvp_flow_stateMachine: state RESVED";
  ProcessedLine pl;
  pl.raw_line = std::string_view(content);

  SECTION("Substring present → passes") {
    std::string needle = "INFO";
    RawLineFilter f(needle);
    REQUIRE(f.passes(&pl));
  }

  SECTION("Substring absent → fails") {
    std::string needle = "ERROR";
    RawLineFilter f(needle);
    REQUIRE_FALSE(f.passes(&pl));
  }

  SECTION("Full string match → passes") {
    std::string needle = content;
    RawLineFilter f(needle);
    REQUIRE(f.passes(&pl));
  }

  SECTION("Longer than raw_line → fails") {
    std::string needle = content + " extra";
    RawLineFilter f(needle);
    REQUIRE_FALSE(f.passes(&pl));
  }

  SECTION("Inverted: substring present → fails") {
    std::string needle = "rsvp";
    RawLineFilter f(needle);
    f.invert();
    REQUIRE_FALSE(f.passes(&pl));
  }

  SECTION("Inverted: substring absent → passes") {
    std::string needle = "TRACE";
    RawLineFilter f(needle);
    f.invert();
    REQUIRE(f.passes(&pl));
  }

  SECTION("Calling passes(ParsedLine*) throws") {
    std::string needle = "INFO";
    RawLineFilter f(needle);
    ParsedLine* parsed = nullptr; // not dereferenced before the throw
    REQUIRE_THROWS_AS(f.passes(parsed), std::runtime_error);
  }
}

// ──────────────────────────────────────────────
// LineNumberFilter
// ──────────────────────────────────────────────

TEST_CASE("LineNumberFilter - line number range") {
  // LineNumberFilter works on ProcessedLine (line_num field), not on ParsedLine.
  ProcessedLine pl;

  SECTION("line_num inside [from, to] → passes") {
    LineNumberFilter f(10, 20);
    pl.line_num = 10; REQUIRE(f.passes(&pl)); // at lower bound
    pl.line_num = 15; REQUIRE(f.passes(&pl)); // in the middle
    pl.line_num = 20; REQUIRE(f.passes(&pl)); // at upper bound
  }

  SECTION("line_num outside [from, to] → fails") {
    LineNumberFilter f(10, 20);
    pl.line_num = 9;  REQUIRE_FALSE(f.passes(&pl)); // just below
    pl.line_num = 21; REQUIRE_FALSE(f.passes(&pl)); // just above
    pl.line_num = 0;  REQUIRE_FALSE(f.passes(&pl));
  }

  SECTION("Single-line range [n, n]") {
    LineNumberFilter f(5, 5);
    pl.line_num = 5; REQUIRE(f.passes(&pl));
    pl.line_num = 4; REQUIRE_FALSE(f.passes(&pl));
    pl.line_num = 6; REQUIRE_FALSE(f.passes(&pl));
  }

  SECTION("Inverted: inside range → fails") {
    LineNumberFilter f(10, 20);
    f.invert();
    pl.line_num = 15;
    REQUIRE_FALSE(f.passes(&pl));
  }

  SECTION("Inverted: outside range → passes") {
    LineNumberFilter f(10, 20);
    f.invert();
    pl.line_num = 5;
    REQUIRE(f.passes(&pl));
  }

  SECTION("Calling passes(ParsedLine*) throws") {
    LineNumberFilter f(1, 100);
    ParsedLine* parsed = nullptr; // not dereferenced before the throw
    REQUIRE_THROWS_AS(f.passes(parsed), std::runtime_error);
  }
}
