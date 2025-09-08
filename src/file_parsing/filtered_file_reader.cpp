#include "filtered_file_reader.hpp"
#include "line_format.hpp"
#include "processed_line.hpp"

#include <iostream>
#include <vector>
#include <deque>

#include "logging.hpp"

#define TRASH_SIZE 1024
static char trash[TRASH_SIZE];

FilteredFileReader::FilteredFileReader(std::string& fname, LineFormat* lf) 
  : m_max_chars_per_line(256), m_lf(lf){

  m_is = std::ifstream(fname, std::ios::in);
  m_line_parser = Parser::fromLineFormat(m_lf);
  m_filter = nullptr;
  m_checkpoints.push_back(m_is.tellg());
  m_curr_line = 0;
}

FilteredFileReader::FilteredFileReader(std::string& fname, LineFormat* lf, LineFilter* filter)
  : FilteredFileReader(fname, lf){
  m_filter = filter;
}

FilteredFileReader::~FilteredFileReader(){
  while(!m_cached_previous.valid_lines.empty()){
    free((void*) m_cached_previous.valid_lines.back().raw_line.data());
    m_cached_previous.valid_lines.pop_back();
  }
  delete m_line_parser;
}

void FilteredFileReader::reset(bool checkpoints_also){
  m_is.seekg(m_checkpoints[0]);
  m_curr_line = 0;
  if(checkpoints_also){
    m_checkpoints.resize(1);
  }
  invalidateCachedResults();
  m_filtered_out_lines.clear();
}

void FilteredFileReader::setFormat(LineFormat* format){
  reset(false);
  m_lf = format;
}

void FilteredFileReader::setFilter(LineFilter* filter){
  reset(false);
  m_filter = filter;
}


void FilteredFileReader::incrCurrLine(){
  m_curr_line++;
  if(m_curr_line % m_checkpoint_dist == 0){
    line_t ncp = m_curr_line / m_checkpoint_dist;
    // Could do sanity check checkpoint[ncp] == is.tellg()
    if(ncp == m_checkpoints.size()){
      m_checkpoints.push_back(m_is.tellg());
    } 
    if(ncp > m_checkpoints.size()){
      // Something went very wrong
    }
    
  }
}

void FilteredFileReader::goToCheckpoint(line_t cp_id){
  m_is.seekg(m_checkpoints[cp_id]);
  m_curr_line = m_checkpoint_dist * cp_id;
}

size_t FilteredFileReader::readRawLine(char* s, uint32_t max_chars){
  m_is.getline(s, max_chars);
  // if line is "aaa\n" then "aaa\0" will be stored in s, but gcount still return 4
  s[max_chars-1] = 0;
  size_t nread = m_is.gcount();
  while(!m_is.eof() && m_is.fail()){
    m_is.getline(trash, TRASH_SIZE);
  }
  incrCurrLine();
  return nread;
}

void FilteredFileReader::skipNextRawLines(line_t n){
  
  while(n){
    if(m_is.eof()){
      incrCurrLine();
      return;
    }
    m_is.getline(trash, TRASH_SIZE);
    // if line is > than 1024 char, getline sets failbit
    if(m_is.fail()) continue;
    n--;
    incrCurrLine();
  }
}

int FilteredFileReader::addFilteredOutGroup(line_t stt, line_t end, uint64_t idx){
  // Want to just insert but might need to merge left/right/both
  LOG_ENTRY("FilteredFileReader::addFilteredOutGroup");
  LOG_FCT(5, "Adding filtered out group [%llu, %llu] at id %llu\n", stt, end, idx);
  bool need_left_merge = idx > 0 && stt <= m_filtered_out_lines[idx-1].second + 1;
  bool need_right_merge = idx + 1 < m_filtered_out_lines.size() && end >= m_filtered_out_lines[idx].first - 1;

  if(need_left_merge && need_right_merge){
    m_filtered_out_lines[idx-1].second = m_filtered_out_lines[idx].second;
    m_filtered_out_lines.erase(m_filtered_out_lines.begin() + idx);
    return idx-1;
  } else if (need_left_merge) {
    m_filtered_out_lines[idx-1].second = end;
    return idx-1;
  } else if (need_right_merge) {
    m_filtered_out_lines[idx].first = stt;
    return idx;
  } else {
    m_filtered_out_lines.emplace(m_filtered_out_lines.begin() + idx, std::make_pair(stt, end));
    return idx;
  }
  
}

void FilteredFileReader::seekRawLine(line_t num){
  line_t ncp = num / m_checkpoint_dist;
  // TODO allow reading backwards and evaluate time gains
  if(ncp < m_checkpoints.size()){
    if(m_curr_line > num  || m_curr_line < ncp*m_checkpoint_dist){
      goToCheckpoint(ncp);
    }
  } else {
    // m_checkpoint is always at least size 1
    goToCheckpoint(m_checkpoints.size()-1);
  }
  skipNextRawLines(num-m_curr_line);
}

size_t FilteredFileReader::getNextValidLine(char* dest, ProcessedLine& pl, line_t stop_at_line){
  LOG_LOGENTRY(5, "FilteredFileReader::getNextValidLine");
  LOG_FCT(5, "Searching for next valid line from %lu up to %lu\n",  m_curr_line, stop_at_line);

  if(m_curr_line >= stop_at_line) {LOG_EXIT(); return 0;}

  line_t begin_line = m_curr_line;
  // TODO Binary search
  int i = 0;
  while(i < m_filtered_out_lines.size() && m_curr_line > m_filtered_out_lines[i].second){
    i++;
  }
  if(i == m_filtered_out_lines.size()){
    // We are exploring uncharted territory in the file
    // Cannot move forward
  } else {
    // here we know that curr_line <= filtered_lines[i].end AND curr_line > filtered_line[i-1].end
    if(m_curr_line >= m_filtered_out_lines[i].first){
      // Here is a block of line, that was already processed once and we know doesnt pass the filter
      // Just skip ahead
      if(m_filtered_out_lines[i].second >= stop_at_line){
        // Don't move cursor?
        LOG_EXIT();
        return 0;
      }
      seekRawLine(m_filtered_out_lines[i].second + 1);
      
      // used later
      i++;
    } else {
      // We are on a line that doesn't belong to any known filter_out groups
      // Cannot move forward
    }    
  }
  line_t fo_end, fo_begin = m_curr_line;
  bool lb = i == m_filtered_out_lines.size();
  std::streampos line_stt;
  while(!m_is.eof() && (m_curr_line < stop_at_line)){
    line_stt = m_is.tellg();
    fo_end = (m_curr_line == 0) ? 0 : m_curr_line -1;
    LOG_FCT(5, "Looking at line number %llu filtered out start and end: %llu, %llu\n", m_curr_line, fo_begin, fo_end);
    size_t nread = readRawLine(dest, m_max_chars_per_line);
    LOG_FCT(5, "Read %llu chars, line is well formated: %d, line content is %s\n", nread, pl.well_formated, dest);
    pl.set_data(m_curr_line-1, dest, nread, m_line_parser, line_stt);
    if( (m_accept_bad_format && !pl.well_formated) 
        || m_filter == nullptr || m_filter->passes(pl.pl.get())){
      
      if(fo_end > fo_begin){ // Since intervals are [incl; incl], could add new block on ">=", but let's not care about single lines
        addFilteredOutGroup(fo_begin, fo_end, i);
      }
      LOG_EXIT();
      return nread;
    }
    // Line is NOT valid here
    fo_end++;
    if(!lb && fo_end + 1 >= m_filtered_out_lines[i].first){
      
      if(m_filtered_out_lines[i].second >= stop_at_line){
        // Don't move cursor?
        LOG_EXIT();
        return 0;
      }

      i = 1 + addFilteredOutGroup(fo_begin, fo_end, i);
      lb = i == m_filtered_out_lines.size();
      seekRawLine(m_filtered_out_lines[i].second + 1);
      fo_begin = m_curr_line;
    }
  }
  LOG_EXIT();
  return 0;
}




size_t FilteredFileReader::getPreviousValidLine(char* dest, ProcessedLine& pl){
  // Since we can't really read lines backwards, lets optimize a bit
  // We go back (one checkpoint at a time) and read all the lines still unread until curr_line
  // We store all the valid lines we find (usually one previous call is followed by another)
  // The following static variables are here for this
  
  // TODO Maybe hardcode a different value?
  static constexpr size_t max_lines_stored = m_checkpoint_dist;

  line_t& searched_from = m_cached_previous.searched_from;
  line_t& searched_to = m_cached_previous.searched_to;
  std::deque<ProcessedLine>& valid_lines = m_cached_previous.valid_lines;
  
  // higher, as in the document, meaning line count lower
  bool searching_from_higher = false;
  if(m_curr_line <= searched_from && valid_lines.size() > 0){
    if(m_curr_line < searched_to){
      // Requested from too high, don't know any valid lines
      searching_from_higher = true;
    } else {
      for(auto itt = valid_lines.end()-1; itt >= valid_lines.begin(); itt--){
        if(m_curr_line > itt->line_num){
          // Place cursor at beginning of line
          m_is.seekg(itt->stt_pos);
          m_curr_line = itt->line_num;

          //! We dont want to read next line as this call might be call in chain and depends on m_curr_line

          // Copy data
          pl = ProcessedLine(*itt, dest, m_max_chars_per_line);
          return pl.raw_line.size();
        }
      }
    }
  }
  line_t pcp = m_curr_line / m_checkpoint_dist;
  line_t begin_line = m_curr_line;
  line_t line_stop_search = m_curr_line;
  goToCheckpoint(pcp);
  line_t highest_line_searched = m_curr_line;
  bool found_valid = false;
  
  std::deque<ProcessedLine> new_findings;

  char* tmpdest = (char*) malloc(m_max_chars_per_line * sizeof(char));
  ProcessedLine tmp_pl;
  
  while (1) {
    if(tmpdest == nullptr) {
      tmpdest = (char*) malloc(m_max_chars_per_line * sizeof(char));
    }
    size_t nread = getNextValidLine(tmpdest, tmp_pl, line_stop_search);
    if(nread != 0){
      found_valid = true;
      new_findings.emplace_back(tmp_pl);
      // only referece to the char array is now in the string_view of ProcessedLine
      // MUST NOT FORGET TO FREE AT SOME POINT
      tmpdest = nullptr;
    } else {
      // Couldn't read anything, reached end of search space (eol or line_limit)
      if(found_valid){
        // We are searching outside the range that was previously stored ([`searched_from`; `searched_to`])
        // If those results are "not too far", we might be able to keep them by
        // searching the entire space between the current search space and the already searched space
        // This will take time but might save more later down the line
        // The furthest we are, the more time it might take, need to chose between
        //    1. Take a lot of time but keep old results
        //    2. Discard old results stop now
        // The limit between the two is chosen arbitrarily atm
        line_t dist_to_old_results;
        line_t from_line, to_line;
        if(searching_from_higher){
          dist_to_old_results = searched_to - begin_line;
          from_line = begin_line; 
          to_line = searched_to;
        } else {
          dist_to_old_results = highest_line_searched - searched_from;
          from_line = searched_from; 
          to_line = highest_line_searched;
        }
        if(dist_to_old_results < 10*m_checkpoint_dist){
          // Try to keep old results
          seekRawLine(from_line);
          while(1){
            if(tmpdest == nullptr) {
              tmpdest = (char*) malloc(m_max_chars_per_line * sizeof(char));
            }
            nread = getNextValidLine(tmpdest, tmp_pl, to_line);
            if(nread == 0){
              if(searching_from_higher){
                while(!new_findings.empty()){
                  valid_lines.emplace_front(new_findings.back());
                  new_findings.pop_back();
                }
                searched_to = highest_line_searched;
              } else {
                while(!new_findings.empty()){
                  valid_lines.emplace_back(new_findings.front());
                  new_findings.pop_front();
                }
                searched_from = begin_line;
              }
              break;
            } else {
              if(searching_from_higher){
                new_findings.emplace_back(tmp_pl);
              } else {
                new_findings.emplace_front(tmp_pl);
              }
              tmpdest = nullptr;
            }
          }
        
        } else {
          // The two search spaces are too far, discard old results
          while(!valid_lines.empty()){
            // Free all our tmpdests
            free((char*) valid_lines.back().raw_line.data());
            valid_lines.pop_back();
          }
          valid_lines = std::move(new_findings);
          searched_from = begin_line;
          searched_to = highest_line_searched;

        }
        free(tmpdest);
        break;
      } 
      free(tmpdest);
      // Search one checkpoint higher if possible
      if(pcp == 0){
        // Already searched up to top, found no match...
        return 0;
      }
      line_stop_search = pcp * m_checkpoint_dist;
      pcp--;
      goToCheckpoint(pcp);
      highest_line_searched = m_curr_line;
    }
  }
  // This condition should never be triggered, but let's be sure
  if(!found_valid) return 0;

  // We know that the value is somewhere is the store now, easiest way to find it, call the function again
  // OK to set m_curr_line, without seeking as will be reset with ans in the store
  m_curr_line = begin_line;
  return getPreviousValidLine(dest, pl);
  
}


void FilteredFileReader::invalidateCachedResults(){
  m_cached_previous.searched_from = m_cached_previous.searched_to = 0;
  std::deque<ProcessedLine>& valid_lines = m_cached_previous.valid_lines;
  while(!valid_lines.empty()){
    while(!valid_lines.empty()){
      // Free all our tmpdests
      free((char*) valid_lines.back().raw_line.data());
      valid_lines.pop_back();
    }
  }
}