#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "test_helpers.hpp"

TEST_CASE("Line Format specifier parsing") {
  setup();
  std::string spec = "{INT:Date} {INT:Time} {STR:Level} :{CHR:,.,1}{STR:Source}:{CHR:, ,1}{STR:Mesg}";
  std::unique_ptr<LineFormat> lf = LineFormat::fromFormatString(spec);
  lf->toString();
  REQUIRE(lf->fields.size() == 12);
  REQUIRE(dynamic_cast<LineIntField*>(lf->fields[0])     != nullptr);
  REQUIRE(dynamic_cast<WhitespaceField*>(lf->fields[1])  != nullptr); // space in format → WS
  REQUIRE(dynamic_cast<LineIntField*>(lf->fields[2])     != nullptr);
  REQUIRE(dynamic_cast<WhitespaceField*>(lf->fields[3])  != nullptr); // space in format → WS
  REQUIRE(dynamic_cast<LineStrField*>(lf->fields[4])     != nullptr);
  REQUIRE(dynamic_cast<WhitespaceField*>(lf->fields[5])  != nullptr); // space in format → WS
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[6])     != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[7])     != nullptr);
  REQUIRE(dynamic_cast<LineStrField*>(lf->fields[8])     != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[9])    != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[10])    != nullptr);
  REQUIRE(dynamic_cast<LineStrField*>(lf->fields[11])    != nullptr);

  std::shared_ptr<Parser> p  = Parser::fromLineFormat(std::move(lf));
  ParsedLine* pl = new ParsedLine(p->format.get());
  REQUIRE(p->parseLine(info_lines[0], pl) == true);
  delete pl;
  teardown();
}

TEST_CASE("LineFormat - getFieldFromName") {
  setup();
  std::unique_ptr<LineFormat>  lf = getDefaultLineFormat();

  SECTION("Named fields are found and have correct type") {
    LineField* date  = lf->getFieldFromName("Date");
    LineField* time  = lf->getFieldFromName("Time");
    LineField* level = lf->getFieldFromName("Level");
    LineField* src   = lf->getFieldFromName("Source");
    LineField* mesg  = lf->getFieldFromName("Mesg");

    REQUIRE(date  != nullptr);
    REQUIRE(time  != nullptr);
    REQUIRE(level != nullptr);
    REQUIRE(src   != nullptr);
    REQUIRE(mesg  != nullptr);

    REQUIRE(dynamic_cast<LineIntField*>(date)  != nullptr);
    REQUIRE(dynamic_cast<LineIntField*>(time)  != nullptr);
    REQUIRE(dynamic_cast<LineStrField*>(level) != nullptr);
    REQUIRE(dynamic_cast<LineStrField*>(src)   != nullptr);
    REQUIRE(dynamic_cast<LineStrField*>(mesg)  != nullptr);
  }

  SECTION("Unknown name returns nullptr") {
    REQUIRE(lf->getFieldFromName("DoesNotExist") == nullptr);
    REQUIRE(lf->getFieldFromName("date")         == nullptr); // case-sensitive
  }

  SECTION("Empty string (unnamed fields) returns nullptr") {
    // Unnamed fields are not inserted into the name map
    REQUIRE(lf->getFieldFromName("") == nullptr);
  }

  teardown();
}

TEST_CASE("LineFormat - field counts") {
  setup();
  std::unique_ptr<LineFormat>  lf = getDefaultLineFormat();
  // getDefaultLineFormat: 2 INT, 0 DBL, 3 CHR, 3 STR, 4 WS
  REQUIRE(lf->getNIntFields()        == 2);
  REQUIRE(lf->getNDoubleFields()     == 0);
  REQUIRE(lf->getNCharFields()       == 3);
  REQUIRE(lf->getNStringFields()     == 3);
  REQUIRE(lf->getNWhiteSpaceFields() == 4);
  teardown();
}

TEST_CASE("LineFormat - fromFormatString produces WhitespaceField for spaces") {
  setup();
  std::unique_ptr<LineFormat> lf = LineFormat::fromFormatString("{INT:A} {INT:B}");
  REQUIRE(lf->fields.size() == 3);
  REQUIRE(dynamic_cast<LineIntField*>(lf->fields[0])    != nullptr);
  REQUIRE(dynamic_cast<WhitespaceField*>(lf->fields[1]) != nullptr);
  REQUIRE(dynamic_cast<LineIntField*>(lf->fields[2])    != nullptr);
  REQUIRE(lf->getNIntFields()        == 2);
  REQUIRE(lf->getNWhiteSpaceFields() == 1);
  teardown();
}

TEST_CASE("LineFormat - DBL field parsing") {
  setup();
  // Format: INT, whitespace, DBL
  std::string spec = "{INT:Count} {DBL:Score}";
  std::unique_ptr<LineFormat>  lf   = LineFormat::fromFormatString(spec);

  REQUIRE(lf->getNIntFields()        == 1);
  REQUIRE(lf->getNDoubleFields()     == 1);
  REQUIRE(lf->getNCharFields()       == 0);
  REQUIRE(lf->getNWhiteSpaceFields() == 1);

  SECTION("getFieldFromName returns LineDblField") {
    LineField* score = lf->getFieldFromName("Score");
    REQUIRE(score != nullptr);
    REQUIRE(dynamic_cast<LineDblField*>(score) != nullptr);
  }

  SECTION("Parser fills DBL field correctly") {
    std::shared_ptr<Parser> p  = Parser::fromLineFormat(std::move(lf));
    ParsedLine* pl = new ParsedLine(p->format.get());
    std::string line = "42 3.14";
    REQUIRE(p->parseLine(line, pl));
    REQUIRE(*(pl->getIntField(0)) == 42);
    REQUIRE(*(pl->getDblField(0)) == Catch::Approx(3.14));
    delete pl;
  }

  teardown();
}

TEST_CASE("Parser - WhitespaceField parsing") {
  setup();
  auto lf = LineFormat::fromFormatString("{INT:A} {INT:B}");
  auto p  = Parser::fromLineFormat(std::move(lf));
  ParsedLine* pl = new ParsedLine(p->format.get());

  SECTION("single space") {
    REQUIRE(p->parseLine("10 20", pl));
    REQUIRE(*(pl->getIntField(0)) == 10);
    REQUIRE(*(pl->getIntField(1)) == 20);
  }

  SECTION("multiple spaces") {
    REQUIRE(p->parseLine("10   20", pl));
    REQUIRE(*(pl->getIntField(0)) == 10);
    REQUIRE(*(pl->getIntField(1)) == 20);
  }

  SECTION("tab character") {
    REQUIRE(p->parseLine("10\t20", pl));
    REQUIRE(*(pl->getIntField(0)) == 10);
    REQUIRE(*(pl->getIntField(1)) == 20);
  }

  SECTION("mixed whitespace") {
    REQUIRE(p->parseLine("10 \t 20", pl));
    REQUIRE(*(pl->getIntField(0)) == 10);
    REQUIRE(*(pl->getIntField(1)) == 20);
  }

  delete pl;
  teardown();
}
