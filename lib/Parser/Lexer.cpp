/*
 * Copyright (c) 2020, Devin Nakamura
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
#if 0
Quotation* Parser::parse_quote(std::istream& input) {
  // TODO enusre that old stack is rooted
  // TODO is this re-entrant?
  if (input.eof()) {
    return nullptr;
  }
  char c = ' ';
  while()
}

#endif

namespace {
// Helper functions because we dont know what type intptr_t is
template <typename T>
T string_convert(const std::string& str, std::size_t* pos = 0, int base = 10);

template <>
int string_convert(const std::string& str, std::size_t* pos, int base) {
  return std::stoi(str, pos, base);
}
template <>
long string_convert(const std::string& str, std::size_t* pos, int base) {
  return std::stol(str, pos, base);
}
template <>
long long string_convert(const std::string& str, std::size_t* pos, int base) {
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

/**
 *   std::optional<std::string> read_token();
  // pushes either a string or an int on the stack
  void lex_token();
  */

std::optional<std::string> Lexer::token_string() {
restart:
  if (parse_stack_.empty()) {
    return {};
  }
  std::istream* current_stream = parse_stack_.front().get();
  char c = ' ';
  while (std::isspace(c) && !current_stream->eof()) {
    current_stream->get(c);
  }
  if (current_stream->eof()) {
    parse_stack_.pop_front();
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
  buffer += c;
  while (!current_stream->eof()) {
    current_stream->get(c);
    if (std::isspace(c)) {
      break;
    }
    buffer += c;
  }
  // TODO, maybe some kind of diagnostic if there is no whitespace at EOF?
  return std::move(buffer);
}

TokenVariant Lexer::token() {
  auto tok_str = token_string();
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
  const auto tok = token();
  if (std::holds_alternative<std::string>(tok)) {
    const std::string& str = std::get<std::string>(tok);
    vm_.push(vm_.allocate<String>(str.c_str(), str.length()));
  } else if (std::holds_alternative<intptr_t>(tok)) {
    vm_.push(Cell::from_int(std::get<intptr_t>(tok)));
  } else {
    vm_.push(False);
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

  return std::move(buffer);
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
