#ifndef FILTER_PARSING_HPP
#define FILTER_PARSING_HPP

#include "line_filter.hpp"

#include <memory>
#include <string>
#include <tuple>
#include <utility>

std::pair<size_t, BitwiseOp> find_next_bitwise_op(std::string& s, size_t start_pos=0);

std::tuple<size_t, size_t, FilterComparison, bool> find_next_comparator(std::string& s, size_t start_pos=0);

std::shared_ptr<LineFilter> parse_filter_decl(std::string fdecl, LineFormat* lfmt);

#endif
