/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "hustle/Parser/Lexer.hpp"
#include "hustle/Support/Error.hpp"
#include "hustle/VM.hpp"
#include <cctype>
#include <ios>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace hustle;
using namespace std::literals;

namespace {
// Helper functions because we dont know what type intptr_t is
template <typename T>
T string_convert(const std::string& str, std::size_t* pos = 0, int base = 10);

template <>
[[maybe_unused]] int string_convert(const std::string& str, std::size_t* pos,
                                    int base) {
  return std::stoi(str, pos, base);
}
template <>
[[maybe_unused]] long string_convert(const std::string& str, std::size_t* pos,
                                     int base) {
  return std::stol(str, pos, base);
}
template <>
[[maybe_unused]] long long string_convert(const std::string& str,
                                          std::size_t* pos, int base) {
  return std::stoll(str, pos, base);
}
} // namespace

static std::optional<intptr_t> parse_number(const std::string& s) {
  HSTL_ASSERT(s.length() > 0);

  try {
    size_t offset = 0;
    intptr_t value = string_convert<intptr_t>(s, &offset, 0);
    if (offset == s.size()) {
      return value;
    }
    // TODO should we be trying to catch the range exception?
  } catch (std::invalid_argument& e) {
    return {};
  }
  return {};
}

std::optional<std::string> Lexer::token_string(bool force) {
restart:
  if (parse_stack_.empty()) {
    return {};
  }
  std::istream* current_stream = parse_stack_.front().get();
  char c = ' ';
  // Skip over any whitespace characters
  while (std::isspace(c) && !current_stream->eof()) {
    current_stream->get(c);
  }
  if (current_stream->eof()) {
    parse_stack_.pop_front();
    if (!force) {
      return {};
    }
    goto restart;
  }
  if (c == '"') {
    return "\""s;
  }
  if (c == '#') {
    // comment char, skip until end of line
    current_stream->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    goto restart;
  }

  // TODO this can be optimized to be stack allocated most of the time
  std::string buffer;
  buffer.reserve(32);
  do {
    buffer += c;
    current_stream->get(c);
  } while (!current_stream->eof() && !std::isspace(c));

  // TODO, maybe some kind of diagnostic if there is no whitespace at EOF?
  return buffer;
}

TokenVariant Lexer::token(bool force) {
  auto tok_str = token_string(force);
  if (!tok_str) {
    return TokenVariant();
  }
  std::optional<intptr_t> intval = parse_number(*tok_str);
  if (intval) {
    HSTL_ASSERT(*intval <= CELL_INT_MAX);
    HSTL_ASSERT(*intval >= CELL_INT_MIN);
    return *intval;
  } else {
    return std::move(*tok_str);
  }
}

void Lexer::lex_token() {
  const auto tok = token(true);
  if (std::holds_alternative<std::string>(tok)) {
    const std::string& str = std::get<std::string>(tok);
    vm_.push(vm_.allocate<String>(str.c_str(), str.length()));
  } else if (std::holds_alternative<intptr_t>(tok)) {
    vm_.push(Cell::from_int(std::get<intptr_t>(tok)));
  } else {
    // I'm not sure if this can actually happen
    vm_.push(vm_.globals.False);
  }
}

std::string Lexer::read_until(char term) {
  std::string buffer;
  HSTL_ASSERT(!parse_stack_.empty());
  auto* current_stream = parse_stack_.front().get();

  while (true) {
    // TODO error on eof
    char c;
    current_stream->get(c);
    if (c == term) {
      break;
    } else {
      buffer += c;
    }
  }

  return buffer;
}

void Lexer::add_stream(StreamPtr ptr) {
  parse_stack_.emplace_front(std::move(ptr));
}

std::istream* Lexer::current_stream() const {
  if (parse_stack_.empty()) {
    return nullptr;
  } else {
    return parse_stack_.front().get();
  }
}
