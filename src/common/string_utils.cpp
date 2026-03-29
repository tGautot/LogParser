#include "string_utils.hpp"
#include <string>
#include <algorithm>

std::string::iterator find_case_insensitive(std::string to_search, std::string to_find){
  return std::search(to_search.begin(), to_search.end(), to_find.begin(), to_find.end(), 
    [](unsigned char a, unsigned char b){
      return std::tolower(a) == std::tolower(b);
    });
}

std::string_view::iterator find_case_insensitive(std::string_view to_search, std::string_view to_find){
  return std::search(to_search.begin(), to_search.end(), to_find.begin(), to_find.end(), 
    [](unsigned char a, unsigned char b){
      return std::tolower(a) == std::tolower(b);
    });
}

// Trim from the start (in place)
void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// Trim from the end (in place)
void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}