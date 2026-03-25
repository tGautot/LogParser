#ifndef FILTERED_FILE_READER_HPP
#define FILTERED_FILE_READER_HPP

#include "common.hpp"
#include "line_format.hpp"
#include "line_parser.hpp"
#include "line_filter.hpp"
#include "processed_line.hpp"

#include <deque>
#include <memory>


/**
  * FileData
  * Any file opened has some data that doesn't change no matter what the user has 
  * set for line format/filters or other configurable paramter, this class holds this data
  * Hence there should every be 1 instance of this class per file open
  */
typedef struct {
  file_pos_t size;
  std::vector<file_pos_t> line_index; // file_pos_t at which lines start
  std::string fname;
  const char* data; // raw bytes, from file mapping
} FileData;


/**
  * FilterConfig
  * Holds all the elements which can changes how lines can be interpreted/validated
  * Most of these elements should be available to the user to change to his liking
  */
typedef struct {
  std::shared_ptr<Parser> parser; // holds the line_format
  std::shared_ptr<LineFilter> filter;
  bool accept_bad_format;
} FilterFileReaderConfig;

/**
  * FilteredFileData
  * When a file is given some of Parsx configurable elements (line format, filters,...) we can
  * store some additional data tha will help us navigate the file more efficiently, 
  * this class stores this data
  * This also means, any two FilteredFileReader using the same FilterConfig, can share the same FilteredFileData
  */
typedef struct {
  std::vector<bool> line_passes; // std::vector<bool>, caution is advised......................
  std::vector<line_t> valid_line_index; // Translates local line id to global
} FilteredFileData;



/**
  * FilteredFileReader
  * Multiple instances of this class can be spawned targetting the same file but with different configs
  * This can be usefull in scenarios when one wants to search for something in the file without perturbing 
  * the state of another reader
*/
class FilteredFileReader {
public:
  std::shared_ptr<FilterFileReaderConfig> m_config;
  std::shared_ptr<FilteredFileData> m_filtered_file_data;
  FileData& m_file_data; // FilteredFileReader MUST have a FileData, hence ref
  Parser* m_line_parser;
  
  file_pos_t m_cursor;
  line_t m_curr_line;
  
  void jumpToGlobalLine(line_t line_num);
  void jumpToLocalLine(line_t line_num);
  size_t getNextRawLine(const char** s);
  size_t getPrevRawLine(const char** s);
  void skipNextRawLines(line_t n);
  void incrCurrLine();
  int addFilteredOutGroup(line_t stt, line_t end, uint64_t idx);
  

// Main public interface
  FilteredFileReader(std::string& fname);
  FilteredFileReader(std::string& fname, std::unique_ptr<LineFormat> line_format);
  FilteredFileReader(std::string& fname, std::unique_ptr<LineFormat> line_format, std::shared_ptr<LineFilter> filter);

  ~FilteredFileReader();

  void reset();
  void setFormat(std::unique_ptr<LineFormat> format);
  void setFilter(std::shared_ptr<LineFilter> filter);

  void seekRawLine(line_t line_num);

  // These two functions will return the number of character read
  // If the function returns false, that means that there exist no valid/previous line. 
  // In those cases, there is no garentee as to where the cursor might be after the call
  bool getNextValidLine(ProcessedLine& pl);
  bool getPreviousValidLine(ProcessedLine& pl);
  bool eof(){ return m_cursor >= m_file_data.size; }

  // Used when gdbing internals, since gdb cant access mmaped files
};

__attribute__((noinline)) char readCharAtPos(unsigned long long cp);

#endif