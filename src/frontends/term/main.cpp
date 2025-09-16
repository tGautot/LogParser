#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/ioctl.h>

#include <string>

#include "log_parser_interface.hpp"
#include "logging.hpp"


#define CTRL_CHR(c) (c & 0x1f)

#define ESC_CHR '\x1b'
#define ESC_CMD "\x1b["

#define CLEAR_SCREEN() write(STDOUT_FILENO, ESC_CMD "2J", 4);
#define CURSOR_TL() write(STDOUT_FILENO, ESC_CMD "H", 3);


struct terminal_config {
  int cy, cx;
  uint64_t line_offset = 0, frame_num = 0;
  int nrows, ncols;
  bool reached_eof = false;
  struct termios orig_term;
};

struct terminal_config cfg;

LogParserInterface* lpi;

void die(const char* s){
  CLEAR_SCREEN();
  CURSOR_TL();
  perror(s);
  exit(1);
}

void rollbackTerm(){
  // TODO move this somewhere else
  logger_teardown();
  if(write(STDOUT_FILENO, ESC_CMD "?1049l", 8) != 8) die("write rollback alternate buf");
  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &cfg.orig_term) == -1) die("tcsetattr");
}

void setupTerm(){
  if(tcgetattr(STDIN_FILENO, &cfg.orig_term) == -1) die("tcgetattr");
  atexit(rollbackTerm);

  struct termios upd_term = cfg.orig_term;

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

inline char readByte(){
  char c;
  if(read(STDIN_FILENO, &c, 1) == -1) die("read");
  return c;
} 

enum USER_ACTION {
  NONE,
  MOVE_UP,
  MOVE_DOWN,
  MOVE_LEFT,
  MOVE_RIGHT,
  START_SEARCH,
  START_COMMAND,
  END_COMMAND
};

int getUserAction(){
  char c = readByte();
  if(c == CTRL_CHR('q')){
    CLEAR_SCREEN();
    CURSOR_TL();
    exit(0);
  }

  if(c == ESC_CHR){
    char seq[4];
    seq[0] = readByte();
    if(seq[0] == '['){
      seq[1] = readByte();
      switch (seq[1])
      {
      case 'A':
        return MOVE_UP;
      case 'B':
        return MOVE_DOWN;
      case 'C':
        return MOVE_RIGHT;
      case 'D':
        return MOVE_LEFT;
      default:
        return NONE;
      }
    }
  }

  switch (c)
  {
  case 'z':
  case 'w':
  case 'k':
    return MOVE_UP;
  case 's':
  case 'j':
    return MOVE_DOWN;
  case 'a':
  case 'q':
  case 'h':
    return MOVE_LEFT;
  case 'd':
  case 'l': 
    return MOVE_RIGHT;
  default:
    break;
  }
  return NONE;
}

void handleInput(){
  int action = getUserAction();
  switch (action)
  {
  case MOVE_DOWN:
    if(cfg.cy < cfg.nrows - 4*(1-cfg.reached_eof) ){
      cfg.cy++;
    } else {
      if(!cfg.reached_eof) cfg.line_offset++;
    }
    break;
  case MOVE_UP:
    if(cfg.cy > 4 || (cfg.line_offset == 0 && cfg.cy > 0) ){
      cfg.cy--;
    } else {
      if(cfg.line_offset != 0) {
        cfg.reached_eof = false;
        cfg.line_offset--;
      }
    }
    break;
  case MOVE_LEFT:
    if(cfg.cx > 0){
      cfg.cx--;
    } else {
      // TODO scroll text left
    }
    break;
  case MOVE_RIGHT:
    if(cfg.cx < cfg.ncols - 1){
      cfg.cx++;
    } else {
      // TODO scroll text right
    }
    break;
  default:
    break;
  }
}

void drawRows(std::string& todraw){
  todraw += ESC_CMD "K";
  for(int i = 0; i < cfg.nrows-1; i++){
    todraw += "~";
    line_info_t lineinfo = lpi->getLine(i+cfg.line_offset);
    std::string_view fetched_line = lineinfo.line;
    todraw += fetched_line;
    if(fetched_line.size() < cfg.ncols-1){
      todraw += std::string(cfg.ncols-1-fetched_line.size(), ' ');
    }
    todraw += "\r\n";
    if(lineinfo.flags & INFO_EOF){
      cfg.reached_eof = true;
      break;
    }
  } 
  
  
  // Status Line
  char buf[80];
  snprintf(buf, 80, "Status: l%d:c%d frame: %lu", cfg.cy, cfg.cx, cfg.frame_num);
  todraw += buf;
}

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

LineFormat* getDefaultLineFormat(){
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

int main(int argc, char** argv){
  setupTerm();
  logger_setup();
  logger_set_file("/dev/null");
  CLEAR_SCREEN();
  if(argc != 2){
    printf("Usage ./lp_term <file_path>\n");
    return 1;
  }
  char c;
  char buf[32];
  std::string todraw;
  lpi = new LogParserInterface(argv[1], getDefaultLineFormat(), nullptr);
  std::cout << "First line is " << lpi->getLine(0).line << std::endl;
  handleInput();
  
  if(getWindowSize(&cfg.nrows, &cfg.ncols) == -1) die("getWindowSize");
  while (1) {
    cfg.frame_num++;
    //todraw += ESC_CMD "2J";
    todraw = "";
    todraw += ESC_CMD "?25l"; // Disable cursor display
    todraw += ESC_CMD "H"; // Set cursor at top left position
    drawRows(todraw);
    snprintf(buf, 32, ESC_CMD "%d;%dH", cfg.cy+1, cfg.cx+1);
    todraw += buf;
    todraw += ESC_CMD "?25h"; // enable cursor display
    write(STDOUT_FILENO, todraw.data(), todraw.size());
    handleInput();
  }
}

