#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "test_helpers.hpp"

TEST_CASE("Line Format specifier parsing") {
  setup();
  std::string spec = "{INT:Date} {INT:Time} {STR:Level} {CHR:, ,1}:{CHR:,.,1}{STR:Source}:{CHR:, ,1}{STR:Mesg}";
  LineFormat* lf = LineFormat::fromFormatString(spec);
  lf->toString();
  REQUIRE(lf->fields.size() == 13);
  REQUIRE(dynamic_cast<LineIntField*>(lf->fields[0])  != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[1])  != nullptr);
  REQUIRE(dynamic_cast<LineIntField*>(lf->fields[2])  != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[3])  != nullptr);
  REQUIRE(dynamic_cast<LineStrField*>(lf->fields[4])  != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[5])  != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[6])  != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[7])  != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[8])  != nullptr);
  REQUIRE(dynamic_cast<LineStrField*>(lf->fields[9])  != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[10]) != nullptr);
  REQUIRE(dynamic_cast<LineChrField*>(lf->fields[11]) != nullptr);
  REQUIRE(dynamic_cast<LineStrField*>(lf->fields[12]) != nullptr);

  Parser* p  = Parser::fromLineFormat(lf);
  ParsedLine* pl = new ParsedLine(lf);
  REQUIRE(p->parseLine(info_lines[0], pl) == true);

  freeLineFormat(lf);
  teardown();
}

TEST_CASE("LineFormat - getFieldFromName") {
  setup();
  LineFormat* lf = getDefaultLineFormat();

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

  freeLineFormat(lf);
  teardown();
}

TEST_CASE("LineFormat - field counts") {
  setup();
  LineFormat* lf = getDefaultLineFormat();
  // getDefaultLineFormat: 2 INT, 0 DBL, 7 CHR, 3 STR
  REQUIRE(lf->getNIntFields()    == 2);
  REQUIRE(lf->getNDoubleFields() == 0);
  REQUIRE(lf->getNCharFields()   == 7);
  REQUIRE(lf->getNStringFields() == 3);
  freeLineFormat(lf);
  teardown();
}

TEST_CASE("LineFormat - DBL field parsing") {
  setup();
  // Format: INT, space (literal CHR), DBL
  std::string spec = "{INT:Count} {DBL:Score}";
  LineFormat* lf   = LineFormat::fromFormatString(spec);

  REQUIRE(lf->getNIntFields()    == 1);
  REQUIRE(lf->getNDoubleFields() == 1);
  REQUIRE(lf->getNCharFields()   == 1);

  SECTION("getFieldFromName returns LineDblField") {
    LineField* score = lf->getFieldFromName("Score");
    REQUIRE(score != nullptr);
    REQUIRE(dynamic_cast<LineDblField*>(score) != nullptr);
  }

  SECTION("Parser fills DBL field correctly") {
    Parser* parser = Parser::fromLineFormat(lf);
    ParsedLine* pl = new ParsedLine(lf);
    std::string line = "42 3.14";
    REQUIRE(parser->parseLine(line, pl));
    REQUIRE(*(pl->getIntField(0)) == 42);
    REQUIRE(*(pl->getDblField(0)) == Catch::Approx(3.14));
    delete pl;
    delete parser;
  }

  freeLineFormat(lf);
  teardown();
}
