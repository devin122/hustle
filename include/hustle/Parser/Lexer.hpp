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

#ifndef HUSTLE_PARSER_LEXER_HPP
#define HUSTLE_PARSER_LEXER_HPP

#include <array>
#include <deque>

#include <istream>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <variant>
namespace replxx {
class Replxx;
}
namespace hustle {

struct VM;

using TokenVariant = std::variant<std::monostate, std::string, intptr_t>;

class Lexer {
public:
  using StreamPtr = std::unique_ptr<std::istream>;
  Lexer(VM& vm) : vm_(vm) {}

  std::optional<std::string> token_string(bool force = false);
  TokenVariant token(bool force = false);

  std::string read_until(char ch);

  // pushes either a string or an int on the stack
  void lex_token();

  void add_stream(StreamPtr p);

  std::istream* current_stream() const;

private:
  VM& vm_;
  std::list<StreamPtr> parse_stack_;
};

class InteractiveStreamBuff : public std::streambuf {
  static constexpr size_t BUFFER_SIZE = 128;
  // std::array<char, BUFFER_SIZE> buffer_;

public:
  InteractiveStreamBuff();
  void soft_underflow();

protected:
  int_type underflow() override;

private:
  char buffer_[BUFFER_SIZE];
};

class ReplxxStreamBuff : public std::streambuf {
public:
  ReplxxStreamBuff(replxx::Replxx& repl);

  void start();
  void finish();
  void skip_space();

protected:
  const char* readline(const std::string& prompt);
  int_type underflow() override;

private:
  std::string buffer_;
  replxx::Replxx& replxx_;
};
} // namespace hustle
#endif
