#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <string>

std::string::iterator find_case_insensitive(std::string to_search, std::string to_find);

std::string_view::iterator find_case_insensitive(std::string_view to_search, std::string_view to_find);

// Trim from the start (in place)
void ltrim(std::string &s);

// Trim from the end (in place)
void rtrim(std::string &s);

void trim(std::string &s);

#endif