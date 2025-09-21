#include "log_parser_terminal.hpp"


int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;
  if (write(STDOUT_FILENO, ESC_CMD "6n", 4) != 4) return -1;
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';
  printf("\r\n&buf[1]: '%s'\r\n", &buf[1]);
  
  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
  return 0;
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, ESC_CMD "999C" ESC_CMD "999B", 12) != 12) return -1;
    return getCursorPosition(rows, cols);
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

LogParserTerminal::LogParserTerminal(std::string& filename){
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
  
  lpi = new LogParserInterface(filename, lf, nullptr);
  
}

void LogParserTerminal::drawRows(){
  frame_str += ESC_CMD "K";
  for(int i = 0; i < term_state.nrows-1; i++){
    frame_str += "~";
    line_info_t lineinfo = lpi->getLine(i+term_state.line_offset);
    std::string_view fetched_line = lineinfo.line;
    frame_str += fetched_line;
    if(fetched_line.size() < term_state.ncols-1){
      frame_str += std::string(term_state.ncols-1-fetched_line.size(), ' ');
    }
    frame_str += "\r\n";
    if(lineinfo.flags & INFO_EOF){
      term_state.reached_eof = true;
      break;
    }
  } 
  
  
  // Status Line
  char buf[80];
  snprintf(buf, 80, "Status: l%d:c%d frame: %lu", term_state.cy, term_state.cx, term_state.frame_num);
  frame_str += buf;
}


void LogParserTerminal::loop(){
  char buf[32];

  while (1) {
    term_state.frame_num++;
    //frame_str += ESC_CMD "2J";
    frame_str = "";
    frame_str += ESC_CMD "?25l"; // Disable cursor display
    frame_str += ESC_CMD "H"; // Set cursor at top left position
    drawRows();
    snprintf(buf, 32, ESC_CMD "%d;%dH", term_state.cy+1, term_state.cx+1);
    frame_str += buf;
    frame_str += ESC_CMD "?25h"; // enable cursor display
    write(STDOUT_FILENO, frame_str.data(), frame_str.size());
    handleUserAction(getUserAction());
  }
}

inline char readByte(){
  char c;
  if(read(STDIN_FILENO, &c, 1) == -1) return 0; // TODO die("read");
  return c;
}

user_action_t LogParserTerminal::getUserAction(){
  std::string seq = "";
  bool need_next_byte = true;
  while(1){
    if(need_next_byte) seq += readByte();
    bool partial_match = false;
    for(auto mapping : user_input_mappings){
      std::string match = mapping.first;
      if(match.size() < seq.size()) continue;
      if(match.rfind(seq, 0) == 0){
        if(match.size() == seq.size()) return mapping.second;
        partial_match = true; 
      }
    }
    
    need_next_byte = true;
    // We might find a match if we add characters
    if(partial_match) continue;
    // Single character sequence and no match, exit
    if(seq.size() == 1) break;

    // Multi char seq without match, we might have consumed chars
    // needed to match a specific seq, remove first char and retry
    need_next_byte = false;
    seq = seq.substr(1);
  }

  return ACTION_NONE;
}

void LogParserTerminal::handleUserAction(user_action_t action){
  for(ActionCallbackPtr cb : action_cbs){
    cb(action, term_state, lpi);
  }
}