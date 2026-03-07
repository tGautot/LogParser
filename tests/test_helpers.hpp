#pragma once

#include "line_format.hpp"
#include "line_parser.hpp"
#include "line_filter.hpp"
#include "processed_line.hpp"
#include "filtered_file_reader.hpp"
#include "log_parser_interface.hpp"

#include <cstring>
#include <fstream>
#include <string>
#include <string_view>

extern "C" {
  #include "logging.h"
}

#define TEST_FOLDER "../../tests/"

inline void setup() {
  logger_setup();
  logger_set_minlvl(0);
}

inline void teardown() {
  logger_teardown();
}

// The default log format used across most integration tests (for data/sample.log):
//   {INT:Date} {INT:Time} {STR:Level} {CHR:, ,1}:{CHR:,.,1}{STR:Source}:{CHR:, ,1}{STR:Mesg}
inline LineFormat* getDefaultLineFormat() {
  LineFormat* lf = new LineFormat();
  lf->addField(new LineIntField("Date"));
  lf->addField(new LineChrField("", ' ', true));
  lf->addField(new LineIntField("Time"));
  lf->addField(new LineChrField("", ' ', true));
  lf->addField(new LineStrField("Level", StrFieldStopType::DELIM, ' ', 0));
  lf->addField(new LineChrField("", ' ', true));
  lf->addField(new LineChrField("", ':', false));
  lf->addField(new LineChrField("", '.', true));
  lf->addField(new LineStrField("Source", StrFieldStopType::DELIM, ':', 0));
  lf->addField(new LineChrField("", ':', false));
  lf->addField(new LineChrField("", ' ', true));
  lf->addField(new LineStrField("Mesg", StrFieldStopType::DELIM, 0, 0));
  return lf;
}

inline void freeLineFormat(LineFormat* lf) {
  for (int i = 0; i < (int)lf->fields.size(); i++)
    delete lf->fields[i];
  delete lf;
}

// The 10 INFO lines from sample.log (used by FilteredFileReader and interface tests)
static const std::string_view info_lines[10] = {
  "0322 085338 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085352 INFO   :.......rsvp_parse_objects: obj RSVP_HOP hop=9.67.116.99, lih=0",
  "0322 085352 INFO   :.......rsvp_flow_stateMachine: state RESVED, event RESV",
  "0322 085353 INFO   :......router_forward_getOI: Ioctl to query route entry successful",
  "0322 085353 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085409 INFO   :......router_forward_getOI: Ioctl to query route entry successful",
  "0322 085409 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085422 INFO   :.......rsvp_parse_objects: obj RSVP_HOP hop=9.67.116.99, lih=0",
  "0322 085422 INFO   :.......rsvp_flow_stateMachine: state RESVED, event RESV",
  "0322 085424 INFO   :......router_forward_getOI: Ioctl to query route entry successful"
};

// Same lines interleaved with binary-format (non-parseable) lines,
// as returned by LogParserInterface (which includes surrounding context)
static const std::string_view info_and_bf_lines[14] = {
  "0322 085338 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085352 INFO   :.......rsvp_parse_objects: obj RSVP_HOP hop=9.67.116.99, lih=0",
  "0322 085352 INFO   :.......rsvp_flow_stateMachine: state RESVED, event RESV",
  "0322 085353 INFO   :......router_forward_getOI: Ioctl to query route entry successful",
  "0x00 0x01 0x02 0x03 ..Da..Ba",
  "0x04 0x05 0x06 0x07 ..Da..Ba",
  "0x08 0x09 0x0A 0x0B ..Da..Ba",
  "0x0C 0x0D 0x0E 0x0F ..Da..Ba",
  "0322 085353 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085409 INFO   :......router_forward_getOI: Ioctl to query route entry successful",
  "0322 085409 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085422 INFO   :.......rsvp_parse_objects: obj RSVP_HOP hop=9.67.116.99, lih=0",
  "0322 085422 INFO   :.......rsvp_flow_stateMachine: state RESVED, event RESV",
  "0322 085424 INFO   :......router_forward_getOI: Ioctl to query route entry successful"
};

// Maps the nth INFO line (local id) to its global line number in sample.log
inline int count_to_info_line(int id) {
  switch (id) {
    case 0: return 4;
    case 1: return 12;
    case 2: return 14;
    case 3: return 20;
    case 4: return 29;
    case 5: return 36;
    case 6: return 41;
    case 7: return 49;
    case 8: return 51;
    case 9: return 57;
  }
  return -1;
}
