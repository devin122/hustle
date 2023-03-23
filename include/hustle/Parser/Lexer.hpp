/*
 * Copyright (c) 2020, Devin Nakamura
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
