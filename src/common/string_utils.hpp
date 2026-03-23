#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

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

#endif