/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#ifndef HUSTLE_PARSER_BOOTSTRAP_LEXER_HPP
#define HUSTLE_PARSER_BOOTSTRAP_LEXER_HPP

#include <hustle/cell.hpp>

#include <gsl/string_span>
#include <vector>

namespace hustle {
struct VM;
}

namespace hustle::bootstrap {
using string_span = gsl::string_span<gsl::dynamic_extent>;
using Iterator = string_span::iterator;

/// Given a pair of iterators, parse into a list of Cells using bootstrap
/// parsing rules eg quotes use " ' "" character, and quotes and arrays handled
/// inside the parser. no support for parse words
std::vector<Cell> tokenize(Iterator& it, const Iterator& end, VM& vm,
                           char until = 0);

} // namespace hustle::bootstrap
#endif
