/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#ifndef HUSTLE_PARSER_BOOTSTRAP_LEXER_HPP
#define HUSTLE_PARSER_BOOTSTRAP_LEXER_HPP

#include "hustle/VM/Cell.hpp"

#include <string_view>
#include <vector>

namespace hustle {
struct VM;
}

namespace hustle::bootstrap {

// TODO: get rid of this using once we port all code over
using string_span = std::string_view;
using Iterator = std::string_view::iterator;

/// Given a pair of iterators, parse into a list of Cells using bootstrap
/// parsing rules eg quotes use " ' "" character, and quotes and arrays handled
/// inside the parser. no support for parse words
std::vector<Cell> tokenize(Iterator& it, const Iterator& end, VM& vm,
                           char until = 0);

} // namespace hustle::bootstrap
#endif
