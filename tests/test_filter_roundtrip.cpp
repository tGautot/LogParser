#include <catch2/catch_test_macros.hpp>

#include "line_format.hpp"
#include "line_filter.hpp"
#include "line_parser.hpp"
#include "filter_parsing.hpp"
#include "test_helpers.hpp"

#include <memory>
#include <string>

static std::unique_ptr<LineFormat> makeSimpleFormat() {
  return LineFormat::fromFormatString("{INT:Val} {STR:Name}");
}

// ──────────────────────────────────────────────
// to_string
// ──────────────────────────────────────────────

TEST_CASE("to_string - FieldFilter int") {
  setup();
  auto lf = makeSimpleFormat();
  std::string val = "42";
  FieldFilter f(lf.get(), "Val", FilterComparison::EQUAL, val);
  REQUIRE(f.to_string() == "Val EQ 42");
  teardown();
}

TEST_CASE("to_string - FieldFilter string") {
  setup();
  auto lf = makeSimpleFormat();
  std::string val = "hello";
  FieldFilter f(lf.get(), "Name", FilterComparison::CONTAINS, val);
  REQUIRE(f.to_string() == "Name CT hello");
  teardown();
}

TEST_CASE("to_string - FieldFilter string comparators") {
  setup();
  auto lf = makeSimpleFormat();

  SECTION("EQ") {
    std::string val = "hello";
    FieldFilter f(lf.get(), "Name", FilterComparison::EQUAL, val);
    REQUIRE(f.to_string() == "Name EQ hello");
  }
  SECTION("ST") {
    std::string val = "hello";
    FieldFilter f(lf.get(), "Name", FilterComparison::SMALLER, val);
    REQUIRE(f.to_string() == "Name ST hello");
  }
  SECTION("GT") {
    std::string val = "hello";
    FieldFilter f(lf.get(), "Name", FilterComparison::GREATER, val);
    REQUIRE(f.to_string() == "Name GT hello");
  }
  SECTION("GE") {
    std::string val = "hello";
    FieldFilter f(lf.get(), "Name", FilterComparison::GREATER_EQ, val);
    REQUIRE(f.to_string() == "Name GE hello");
  }
  SECTION("SE") {
    std::string val = "hello";
    FieldFilter f(lf.get(), "Name", FilterComparison::SMALLER_EQ, val);
    REQUIRE(f.to_string() == "Name SE hello");
  }
  SECTION("BW") {
    std::string val = "hel";
    FieldFilter f(lf.get(), "Name", FilterComparison::BEGINS_WITH, val);
    REQUIRE(f.to_string() == "Name BW hel");
  }
  SECTION("EW") {
    std::string val = "llo";
    FieldFilter f(lf.get(), "Name", FilterComparison::ENDS_WITH, val);
    REQUIRE(f.to_string() == "Name EW llo");
  }
  teardown();
}

TEST_CASE("to_string - FieldFilter case-insensitive") {
  setup();
  auto lf = makeSimpleFormat();
  std::string val = "HELLO";
  FieldFilter f(lf.get(), "Name", FilterComparison::EQUAL, val, true);
  REQUIRE(f.to_string() == "Name EQ_CI hello");
  teardown();
}

TEST_CASE("to_string - LineNumberFilter") {
  setup();
  LineNumberFilter f(5, 10);
  REQUIRE(f.to_string() == "line_num CT 5,10");
  teardown();
}

TEST_CASE("to_string - CombinedFilter simple") {
  setup();
  auto lf = makeSimpleFormat();
  std::string v1 = "42", v2 = "hello";
  auto f1 = std::make_shared<FieldFilter>(lf.get(), "Val", FilterComparison::EQUAL, v1);
  auto f2 = std::make_shared<FieldFilter>(lf.get(), "Name", FilterComparison::EQUAL, v2);
  CombinedFilter cf(f1, f2, BitwiseOp::AND);
  REQUIRE(cf.to_string() == "(Val EQ 42) AND (Name EQ hello)");
  teardown();
}

TEST_CASE("to_string - CombinedFilter operators") {
  setup();
  auto lf = makeSimpleFormat();
  std::string v1 = "42", v2 = "hello";
  auto f1 = std::make_shared<FieldFilter>(lf.get(), "Val", FilterComparison::EQUAL, v1);
  auto f2 = std::make_shared<FieldFilter>(lf.get(), "Name", FilterComparison::EQUAL, v2);

  SECTION("OR")  { REQUIRE(CombinedFilter(f1, f2, BitwiseOp::OR).to_string()  == "(Val EQ 42) OR (Name EQ hello)"); }
  SECTION("XOR") { REQUIRE(CombinedFilter(f1, f2, BitwiseOp::XOR).to_string() == "(Val EQ 42) XOR (Name EQ hello)"); }
  SECTION("NOR") { REQUIRE(CombinedFilter(f1, f2, BitwiseOp::NOR).to_string() == "(Val EQ 42) NOR (Name EQ hello)"); }
  teardown();
}

TEST_CASE("to_string - CombinedFilter parenthesizes nested combined") {
  setup();
  auto lf = makeSimpleFormat();
  std::string v1 = "1", v2 = "x", v3 = "2";
  auto f1 = std::make_shared<FieldFilter>(lf.get(), "Val", FilterComparison::EQUAL, v1);
  auto f2 = std::make_shared<FieldFilter>(lf.get(), "Name", FilterComparison::EQUAL, v2);
  auto f3 = std::make_shared<FieldFilter>(lf.get(), "Val", FilterComparison::EQUAL, v3);

  SECTION("Left child is CombinedFilter") {
    auto inner = std::make_shared<CombinedFilter>(f1, f2, BitwiseOp::OR);
    CombinedFilter outer(inner, f3, BitwiseOp::AND);
    REQUIRE(outer.to_string() == "((Val EQ 1) OR (Name EQ x)) AND (Val EQ 2)");
  }
  SECTION("Right child is CombinedFilter") {
    auto inner = std::make_shared<CombinedFilter>(f2, f3, BitwiseOp::AND);
    CombinedFilter outer(f1, inner, BitwiseOp::OR);
    REQUIRE(outer.to_string() == "(Val EQ 1) OR ((Name EQ x) AND (Val EQ 2))");
  }
  SECTION("Both children are CombinedFilter") {
    auto left = std::make_shared<CombinedFilter>(f1, f2, BitwiseOp::OR);
    auto right = std::make_shared<CombinedFilter>(f2, f3, BitwiseOp::AND);
    CombinedFilter outer(left, right, BitwiseOp::XOR);
    REQUIRE(outer.to_string() == "((Val EQ 1) OR (Name EQ x)) XOR ((Name EQ x) AND (Val EQ 2))");
  }
  teardown();
}

// ──────────────────────────────────────────────
// equals
// ──────────────────────────────────────────────

TEST_CASE("equals - same FieldFilter") {
  setup();
  auto lf = makeSimpleFormat();
  std::string v1 = "42", v2 = "42";
  FieldFilter f1(lf.get(), "Val", FilterComparison::EQUAL, v1);
  FieldFilter f2(lf.get(), "Val", FilterComparison::EQUAL, v2);
  REQUIRE(f1.equals(f2));
  REQUIRE(f1 == f2);
  teardown();
}

TEST_CASE("equals - different FieldFilter value") {
  setup();
  auto lf = makeSimpleFormat();
  std::string v1 = "42", v2 = "99";
  FieldFilter f1(lf.get(), "Val", FilterComparison::EQUAL, v1);
  FieldFilter f2(lf.get(), "Val", FilterComparison::EQUAL, v2);
  REQUIRE_FALSE(f1.equals(f2));
  REQUIRE(f1 != f2);
  teardown();
}

TEST_CASE("equals - different FieldFilter comparator") {
  setup();
  auto lf = makeSimpleFormat();
  std::string v1 = "42", v2 = "42";
  FieldFilter f1(lf.get(), "Val", FilterComparison::EQUAL, v1);
  FieldFilter f2(lf.get(), "Val", FilterComparison::GREATER, v2);
  REQUIRE_FALSE(f1.equals(f2));
  teardown();
}

TEST_CASE("equals - different filter types") {
  setup();
  auto lf = makeSimpleFormat();
  std::string v = "42";
  FieldFilter ff(lf.get(), "Val", FilterComparison::EQUAL, v);
  LineNumberFilter lnf(5, 10);
  REQUIRE_FALSE(ff.equals(lnf));
  REQUIRE_FALSE(lnf.equals(ff));
  teardown();
}

TEST_CASE("equals - LineNumberFilter") {
  setup();
  LineNumberFilter f1(5, 10);
  LineNumberFilter f2(5, 10);
  LineNumberFilter f3(5, 20);
  REQUIRE(f1.equals(f2));
  REQUIRE_FALSE(f1.equals(f3));
  teardown();
}

TEST_CASE("equals - CombinedFilter") {
  setup();
  auto lf = makeSimpleFormat();
  std::string v1 = "42", v2 = "hello", v3 = "42", v4 = "hello";
  auto a1 = std::make_shared<FieldFilter>(lf.get(), "Val", FilterComparison::EQUAL, v1);
  auto b1 = std::make_shared<FieldFilter>(lf.get(), "Name", FilterComparison::EQUAL, v2);
  auto a2 = std::make_shared<FieldFilter>(lf.get(), "Val", FilterComparison::EQUAL, v3);
  auto b2 = std::make_shared<FieldFilter>(lf.get(), "Name", FilterComparison::EQUAL, v4);

  CombinedFilter cf1(a1, b1, BitwiseOp::AND);
  CombinedFilter cf2(a2, b2, BitwiseOp::AND);
  CombinedFilter cf3(a2, b2, BitwiseOp::OR);
  REQUIRE(cf1.equals(cf2));
  REQUIRE_FALSE(cf1.equals(cf3));
  teardown();
}

TEST_CASE("equals - RawLineFilter") {
  setup();
  std::string s1 = "foo", s2 = "foo", s3 = "bar";
  RawLineFilter f1(s1);
  RawLineFilter f2(s2);
  RawLineFilter f3(s3);
  REQUIRE(f1.equals(f2));
  REQUIRE_FALSE(f1.equals(f3));
  teardown();
}

TEST_CASE("operator== checks inversion") {
  setup();
  auto lf = makeSimpleFormat();
  std::string v1 = "42", v2 = "42";
  FieldFilter f1(lf.get(), "Val", FilterComparison::EQUAL, v1);
  FieldFilter f2(lf.get(), "Val", FilterComparison::EQUAL, v2);
  REQUIRE(f1 == f2);

  f2.invert();
  REQUIRE(f1.equals(f2));
  REQUIRE_FALSE(f1 == f2);
  teardown();
}

// ──────────────────────────────────────────────
// Round-trip: parse -> to_string -> parse -> equals
// ──────────────────────────────────────────────

TEST_CASE("Round-trip - FieldFilter int") {
  setup();
  auto lf = makeSimpleFormat();
  std::string input = "Val EQ 42";
  auto f1 = parse_filter_decl(input, lf.get());
  std::string serialized = f1->to_string();
  REQUIRE(serialized == "Val EQ 42");
  auto f2 = parse_filter_decl(serialized, lf.get());
  REQUIRE(f1->equals(*f2));
  teardown();
}

TEST_CASE("Round-trip - FieldFilter string") {
  setup();
  auto lf = makeSimpleFormat();
  std::string input = "Name CT hello";
  auto f1 = parse_filter_decl(input, lf.get());
  std::string serialized = f1->to_string();
  REQUIRE(serialized == "Name CT hello");
  auto f2 = parse_filter_decl(serialized, lf.get());
  REQUIRE(f1->equals(*f2));
  teardown();
}

TEST_CASE("Round-trip - FieldFilter case-insensitive") {
  setup();
  auto lf = makeSimpleFormat();
  std::string input = "Name EQ_CI HELLO";
  auto f1 = parse_filter_decl(input, lf.get());
  std::string serialized = f1->to_string();
  REQUIRE(serialized == "Name EQ_CI hello");
  auto f2 = parse_filter_decl(serialized, lf.get());
  REQUIRE(f1->equals(*f2));
  teardown();
}

TEST_CASE("Round-trip - FieldFilter all comparators") {
  setup();
  auto lf = makeSimpleFormat();

  SECTION("ST") {
    auto f1 = parse_filter_decl("Val ST 100", lf.get());
    auto f2 = parse_filter_decl(f1->to_string(), lf.get());
    REQUIRE(f1->equals(*f2));
  }
  SECTION("GT") {
    auto f1 = parse_filter_decl("Val GT 10", lf.get());
    auto f2 = parse_filter_decl(f1->to_string(), lf.get());
    REQUIRE(f1->equals(*f2));
  }
  SECTION("SE") {
    auto f1 = parse_filter_decl("Val SE 100", lf.get());
    auto f2 = parse_filter_decl(f1->to_string(), lf.get());
    REQUIRE(f1->equals(*f2));
  }
  SECTION("GE") {
    auto f1 = parse_filter_decl("Val GE 10", lf.get());
    auto f2 = parse_filter_decl(f1->to_string(), lf.get());
    REQUIRE(f1->equals(*f2));
  }
  SECTION("BW") {
    auto f1 = parse_filter_decl("Name BW hel", lf.get());
    auto f2 = parse_filter_decl(f1->to_string(), lf.get());
    REQUIRE(f1->equals(*f2));
  }
  SECTION("EW") {
    auto f1 = parse_filter_decl("Name EW llo", lf.get());
    auto f2 = parse_filter_decl(f1->to_string(), lf.get());
    REQUIRE(f1->equals(*f2));
  }
  teardown();
}

TEST_CASE("Round-trip - LineNumberFilter") {
  setup();
  auto lf = makeSimpleFormat();
  std::string input = "line_num CT 5,10";
  auto f1 = parse_filter_decl(input, lf.get());
  std::string serialized = f1->to_string();
  REQUIRE(serialized == "line_num CT 5,10");
  auto f2 = parse_filter_decl(serialized, lf.get());
  REQUIRE(f1->equals(*f2));
  teardown();
}

TEST_CASE("Round-trip - CombinedFilter AND") {
  setup();
  auto lf = makeSimpleFormat();
  std::string input = "Name EQ hello AND Val EQ 42";
  auto f1 = parse_filter_decl(input, lf.get());
  std::string serialized = f1->to_string();
  auto f2 = parse_filter_decl(serialized, lf.get());
  REQUIRE(f1->equals(*f2));
  teardown();
}

TEST_CASE("Round-trip - CombinedFilter OR") {
  setup();
  auto lf = makeSimpleFormat();
  std::string input = "Name EQ hello OR Val EQ 42";
  auto f1 = parse_filter_decl(input, lf.get());
  std::string serialized = f1->to_string();
  auto f2 = parse_filter_decl(serialized, lf.get());
  REQUIRE(f1->equals(*f2));
  teardown();
}

TEST_CASE("Round-trip - CombinedFilter with parentheses") {
  setup();
  auto lf = makeSimpleFormat();

  SECTION("Left grouping") {
    std::string input = "(Name EQ hello OR Val EQ 99) AND Val EQ 42";
    auto f1 = parse_filter_decl(input, lf.get());
    std::string serialized = f1->to_string();
    auto f2 = parse_filter_decl(serialized, lf.get());
    REQUIRE(f1->equals(*f2));
  }
  SECTION("Right grouping") {
    std::string input = "Val EQ 42 AND (Name EQ hello OR Val EQ 99)";
    auto f1 = parse_filter_decl(input, lf.get());
    std::string serialized = f1->to_string();
    auto f2 = parse_filter_decl(serialized, lf.get());
    REQUIRE(f1->equals(*f2));
  }
  SECTION("Both sides grouped") {
    std::string input = "(Val EQ 1 OR Name EQ x) XOR (Name EQ y AND Val EQ 2)";
    auto f1 = parse_filter_decl(input, lf.get());
    std::string serialized = f1->to_string();
    auto f2 = parse_filter_decl(serialized, lf.get());
    REQUIRE(f1->equals(*f2));
  }
  teardown();
}

TEST_CASE("Round-trip - to_string is idempotent") {
  setup();
  auto lf = makeSimpleFormat();
  std::string input = "(Name EQ hello OR Val EQ 99) AND Val EQ 42";
  auto f1 = parse_filter_decl(input, lf.get());
  std::string s1 = f1->to_string();
  auto f2 = parse_filter_decl(s1, lf.get());
  std::string s2 = f2->to_string();
  REQUIRE(s1 == s2);
  teardown();
}

TEST_CASE("Round-trip - long alias normalizes to short tag") {
  setup();
  auto lf = makeSimpleFormat();

  SECTION("EQUAL -> EQ") {
    auto f = parse_filter_decl("Name EQUAL hello", lf.get());
    REQUIRE(f->to_string() == "Name EQ hello");
  }
  SECTION("GREATER_THAN -> GT") {
    auto f = parse_filter_decl("Val GREATER_THAN 10", lf.get());
    REQUIRE(f->to_string() == "Val GT 10");
  }
  SECTION("CONTAINS -> CT") {
    auto f = parse_filter_decl("Name CONTAINS hello", lf.get());
    REQUIRE(f->to_string() == "Name CT hello");
  }
  SECTION("BEGINS_WITH -> BW") {
    auto f = parse_filter_decl("Name BEGINS_WITH hel", lf.get());
    REQUIRE(f->to_string() == "Name BW hel");
  }
  SECTION("ENDS_WITH -> EW") {
    auto f = parse_filter_decl("Name ENDS_WITH llo", lf.get());
    REQUIRE(f->to_string() == "Name EW llo");
  }
  teardown();
}

// ──────────────────────────────────────────────
// Complex parenthesized expressions
// ──────────────────────────────────────────────

TEST_CASE("Round-trip - triple AND chain") {
  setup();
  auto lf = makeSimpleFormat();
  // A AND B AND C -> parser finds first AND -> left=A, right="B AND C"
  // right recurses -> left=B, right=C -> Combined(B,C,AND)
  // top = Combined(A, Combined(B,C,AND), AND)
  std::string input = "Val EQ 1 AND Name EQ x AND Val EQ 2";
  auto f1 = parse_filter_decl(input, lf.get());
  std::string serialized = f1->to_string();
  auto f2 = parse_filter_decl(serialized, lf.get());
  REQUIRE(f1->equals(*f2));
  teardown();
}

TEST_CASE("Round-trip - triple mixed operators") {
  setup();
  auto lf = makeSimpleFormat();
  // A OR B AND C -> parser finds OR first (earlier pos) -> left=A, right="B AND C"
  std::string input = "Val EQ 1 OR Name EQ x AND Val EQ 2";
  auto f1 = parse_filter_decl(input, lf.get());
  std::string serialized = f1->to_string();
  auto f2 = parse_filter_decl(serialized, lf.get());
  REQUIRE(f1->equals(*f2));
  teardown();
}

TEST_CASE("Round-trip - parentheses override natural parse order") {
  setup();
  auto lf = makeSimpleFormat();
  // Without parens "A OR B AND C" groups as Combined(A, Combined(B,C,AND), OR)
  // With parens "(A OR B) AND C" groups as Combined(Combined(A,B,OR), C, AND)
  // These should NOT be equal
  auto f_no_parens = parse_filter_decl("Val EQ 1 OR Name EQ x AND Val EQ 2", lf.get());
  auto f_with_parens = parse_filter_decl("(Val EQ 1 OR Name EQ x) AND Val EQ 2", lf.get());
  REQUIRE_FALSE(f_no_parens->equals(*f_with_parens));

  // Both should individually round-trip fine
  auto f_no_rt = parse_filter_decl(f_no_parens->to_string(), lf.get());
  auto f_with_rt = parse_filter_decl(f_with_parens->to_string(), lf.get());
  REQUIRE(f_no_parens->equals(*f_no_rt));
  REQUIRE(f_with_parens->equals(*f_with_rt));
  teardown();
}

TEST_CASE("Round-trip - deeply nested parentheses") {
  setup();
  auto lf = makeSimpleFormat();

  SECTION("3 levels deep") {
    // ((A AND B) OR C) XOR D
    std::string input = "((Val EQ 1 AND Name EQ x) OR Val EQ 2) XOR Name EQ y";
    auto f1 = parse_filter_decl(input, lf.get());
    std::string serialized = f1->to_string();
    auto f2 = parse_filter_decl(serialized, lf.get());
    REQUIRE(f1->equals(*f2));
  }
  SECTION("3 levels deep right-leaning") {
    // A XOR (B OR (C AND D))
    std::string input = "Val EQ 1 XOR (Name EQ x OR (Val EQ 2 AND Name EQ y))";
    auto f1 = parse_filter_decl(input, lf.get());
    std::string serialized = f1->to_string();
    auto f2 = parse_filter_decl(serialized, lf.get());
    REQUIRE(f1->equals(*f2));
  }
  SECTION("Balanced tree") {
    // (A AND B) XOR (C AND D)
    std::string input = "(Val EQ 1 AND Name EQ x) XOR (Val EQ 2 AND Name EQ y)";
    auto f1 = parse_filter_decl(input, lf.get());
    std::string serialized = f1->to_string();
    auto f2 = parse_filter_decl(serialized, lf.get());
    REQUIRE(f1->equals(*f2));
  }
  teardown();
}

TEST_CASE("Round-trip - redundant parentheses stripped") {
  setup();
  auto lf = makeSimpleFormat();
  // Extra wrapping parens should be stripped by parser, same result
  auto f1 = parse_filter_decl("((Val EQ 42))", lf.get());
  auto f2 = parse_filter_decl("Val EQ 42", lf.get());
  REQUIRE(f1->equals(*f2));
  teardown();
}

TEST_CASE("Round-trip - combined with line_num") {
  setup();
  auto lf = makeSimpleFormat();
  std::string input = "Name EQ hello AND line_num CT 10,50";
  auto f1 = parse_filter_decl(input, lf.get());
  std::string serialized = f1->to_string();
  auto f2 = parse_filter_decl(serialized, lf.get());
  REQUIRE(f1->equals(*f2));
  teardown();
}

TEST_CASE("Round-trip - 4 filters chained") {
  setup();
  auto lf = makeSimpleFormat();
  std::string input = "Val EQ 1 AND Name EQ x OR Val EQ 2 AND Name EQ y";
  auto f1 = parse_filter_decl(input, lf.get());
  std::string serialized = f1->to_string();
  auto f2 = parse_filter_decl(serialized, lf.get());
  REQUIRE(f1->equals(*f2));
  // Also verify idempotency
  std::string s2 = f2->to_string();
  REQUIRE(serialized == s2);
  teardown();
}

TEST_CASE("Round-trip - complex mixed with case-insensitive") {
  setup();
  auto lf = makeSimpleFormat();
  std::string input = "(Name CT_CI hello OR Val GT 10) AND Name BW_CI wor";
  auto f1 = parse_filter_decl(input, lf.get());
  std::string serialized = f1->to_string();
  auto f2 = parse_filter_decl(serialized, lf.get());
  REQUIRE(f1->equals(*f2));
  teardown();
}
