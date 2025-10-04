#include "log_parser_terminal.hpp"
#include "processed_line.hpp"

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>

extern "C"{
#include "logging.h"
}
#include "logging.hpp"
#include "terminal_state.hpp"

#define CTRL_CHR(c) (c & 0x1f)

#define ESC_CHR '\x1b'
#define ESC_CMD "\x1b["

#define CLEAR_SCREEN() write(STDOUT_FILENO, ESC_CMD "2J", 4);
#define CURSOR_TL() write(STDOUT_FILENO, ESC_CMD "H", 3);



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

static struct termios orig_term;
static void die(const char* s);

static void rollbackTerm(){
  if(write(STDOUT_FILENO, ESC_CMD "?1049l", 8) != 8) die("write rollback alternate buf");
  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term) == -1) die("tcsetattr");
}

static void die(const char* s){
  CLEAR_SCREEN();
  CURSOR_TL();
  rollbackTerm();
  perror(s);
  exit(1);
}

static void setupTerm(term_state_t stt){
  if(tcgetattr(STDIN_FILENO, &orig_term) == -1) die("tcgetattr");
  atexit(rollbackTerm);

  struct termios upd_term = orig_term;

  // No echo: don't print what user is typing
  // No canonical mode: read char by char (don't wait for enter key)
  // No sig: don't allow ctrl+c/z to stop the program
  // No exten: disable ctrl+v escaping next input
  upd_term.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

  // No crnl: Don't map ctrl+m ('\r') to '\n'
  // No ixon: Hopefully you won't run this program on a printer
  upd_term.c_iflag &= ~(ICRNL | IXON | BRKINT | INPCK | ISTRIP);

  // Disable any kind of ouput processing
  upd_term.c_oflag &= ~(OPOST);

  upd_term.c_cflag |= (CS8);


  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &upd_term) == -1) die("tcsetattr");

  if(write(STDOUT_FILENO, ESC_CMD "?1049h", 8) != 8) die("write alternate buf");
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
  setupTerm(term_state);
  atexit(rollbackTerm);
  term_state.cx = term_state.cy = 0;
  term_state.input_mode = InputMode::ACTION;
  term_state.raw_input = "";
}

void LogParserTerminal::registerUserInputMapping(std::string input_seq, user_action_t action_code){
  user_input_mappings.push_back({input_seq, action_code});
}

void LogParserTerminal::registerActionCallback(ActionCallbackPtr action_cb){
  action_cbs.push_back(action_cb);
}

void LogParserTerminal::registerCommandCallback(CommandCallbackPtr cmd_cb){
  command_cbs.push_back(cmd_cb);
}

void LogParserTerminal::drawRows(){
  frame_str += ESC_CMD "K";
  term_state.displayed_pls.clear();
  for(int i = 0; i < term_state.nrows-1; i++){
    frame_str += "~";
    line_info_t lineinfo = lpi->getLine(i+term_state.line_offset);
    term_state.displayed_pls.push_back(lineinfo.line);
    if(lineinfo.line != nullptr) term_state.info_col_size = 2/*size of "~ "*/ + std::to_string(lineinfo.line->line_num).size(); 
  } 
  
  std::string_view fetched_line;
  for(int i = 0; i < term_state.nrows-1; i++){
    const ProcessedLine* pl = term_state.displayed_pls[i];
    int line_size = 0;
    if(pl != nullptr){
      fetched_line = pl->raw_line;
      std::string line_num_str = std::to_string(pl->line_num);
      if(line_num_str.size()+2 < term_state.info_col_size) {
        frame_str += std::string(term_state.info_col_size - line_num_str.size() - 2, ' ');
      }
      frame_str += line_num_str + "~ ";
      std::string formatted_line = std::string(fetched_line);
      size_t match_pos = 0;
      if(term_state.highlight_word != "") while((match_pos = formatted_line.find(term_state.highlight_word, match_pos)) != std::string::npos){
        static std::string end_invert_tag = "\e[27m"; 
        static std::string start_invert_tag = "\e[7m"; 
        LOG(5, "Found one match in line %lu highlighting it...", pl->line_num);
        formatted_line.insert(formatted_line.begin() + match_pos + term_state.highlight_word.length(), end_invert_tag.begin(), end_invert_tag.end());
        formatted_line.insert(formatted_line.begin() + match_pos, start_invert_tag.begin(), start_invert_tag.end());
        match_pos += term_state.highlight_word.length() + start_invert_tag.length() + end_invert_tag.length();
      }

      frame_str += formatted_line;
      if(fetched_line.size() + term_state.info_col_size < term_state.ncols){
        frame_str += std::string(term_state.ncols-fetched_line.size()-term_state.info_col_size, ' ');
      }
    } else {
      frame_str += std::string(term_state.ncols, ' ');
    }
    
    frame_str += "\r\n";
    if(pl == nullptr){
      term_state.reached_eof = true;
    }
  }
  
  char buf[81];
  if(term_state.input_mode == ACTION){
    // Status Line
    snprintf(buf, 80, "Status: l%d:c%d frame: %lu", term_state.cy, term_state.cx, term_state.frame_num);
  } else if(term_state.input_mode == RAW) {
    snprintf(buf, 80, "%s", term_state.raw_input.data());
  } else {
    snprintf(buf, 80, "Unknown input mode %d", term_state.input_mode);
  }
  frame_str += buf;
  if(strlen(buf) < term_state.ncols){
    frame_str += std::string(term_state.ncols-strlen(buf), ' ');
  }
}


void LogParserTerminal::loop(){
  char buf[32];

  while (1) {
    term_state.frame_num++;
    //frame_str += ESC_CMD "2J";
    getWindowSize(&term_state.nrows, &term_state.ncols);
    frame_str = "";
    frame_str += ESC_CMD "?25l"; // Disable cursor display
    frame_str += ESC_CMD "H"; // Set cursor at top left position
    drawRows();
    if(term_state.input_mode == RAW){
      snprintf(buf, 32, ESC_CMD "%d;%dH", term_state.nrows, term_state.raw_input.length()+1);
    } else {
      snprintf(buf, 32, ESC_CMD "%d;%dH", term_state.cy+1, term_state.cx+1);
    }
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
  LOG_LOGENTRY(3, "LogParserTerminal::getUserAction");
  std::string seq = "";
  bool need_next_byte = true;
  
  while(1){
    if(term_state.input_mode == RAW){
      char c = readByte();
      LOG(5, "In raw mode and read char %d\n", c);
      LOG(5, "Before applying char str is %s\n", term_state.raw_input.data());
      if(c == 13) { // <Enter>
        for(auto cmd_cb : command_cbs){
          cmd_cb(term_state.raw_input, term_state, lpi);
        }
        term_state.input_mode = ACTION;
        term_state.raw_input = "";
        break;
      }
      if(c == 127) { // <Backspace>
        term_state.raw_input.erase(term_state.raw_input.size()-1);
        break;
      }
      term_state.raw_input += c;
      break;
    }
    
    if(need_next_byte) seq += readByte();
    if(seq == ":"){
      term_state.input_mode = RAW;
      term_state.raw_input = ":";
      break;
    }
    if(seq == "q") exit(0);
    bool partial_match = false;
    LOG_FCT(3, "Trying to match input sequence %s against %d mappings\n", seq.data(), user_input_mappings.size());
    for(auto mapping : user_input_mappings){
      std::string match = mapping.first;
      if(match.size() < seq.size()) continue;
      if(match.rfind(seq, 0) == 0){
        if(match.size() == seq.size()){
          LOG_FCT(3, "Found match, action id is %d\n", mapping.second);
          LOG_EXIT();
          return mapping.second;
        }
        partial_match = true; 
      }
    }
    
    LOG_FCT(3, "found no match, patial match: %d\n", partial_match);

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
  term_state.current_action_multiplier = 1;
}
