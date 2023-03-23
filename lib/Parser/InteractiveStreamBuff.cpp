/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <cctype>
#include <hustle/Parser/Lexer.hpp>
#include <iostream>
#include <replxx.hxx>
using namespace hustle;

using namespace std::literals;

static const std::string PS1 = "> "s;
static const std::string PS2 = ">> "s;

InteractiveStreamBuff::InteractiveStreamBuff() {
  auto read_size = std::cin.readsome(buffer_, sizeof(buffer_));
  setg(buffer_, buffer_, buffer_ + read_size);
}

int InteractiveStreamBuff::underflow() {
  if (gptr() < egptr()) {
    return *gptr();
  }
  auto read_size = std::cin.readsome(buffer_, sizeof(buffer_));
  if (read_size > 0) {
    setg(buffer_, buffer_, buffer_ + read_size);

  } else {
    // We need to prompt for more input;
    std::cout << ">> ";
    if (std::char_traits<char>::eof() == std::cin.peek()) {
      return std::char_traits<char>::eof();
    }
    read_size = std::cin.readsome(buffer_, sizeof(buffer_));
    setg(buffer_, buffer_, buffer_ + read_size);
  }
  return buffer_[0];
}

void InteractiveStreamBuff::soft_underflow() {}
// TODO should we try returning chars to stdin on destruction

ReplxxStreamBuff::ReplxxStreamBuff(replxx::Replxx& r) : replxx_(r) {
  setg(buffer_.data(), buffer_.data(), buffer_.data());
}

int ReplxxStreamBuff::underflow() {
  if (gptr() < egptr()) {
    return *gptr();
  }
  buffer_.clear();

  buffer_ = readline(PS2);
  buffer_ += "\n";

  setg(buffer_.data(), buffer_.data(), buffer_.data() + buffer_.length());
  return buffer_[0];
}

const char* ReplxxStreamBuff::readline(const std::string& prompt) {
  const char* line = nullptr;
  do {
    line = replxx_.input(prompt);
  } while ((line == nullptr) && (errno == EAGAIN));
  return line;
}

void ReplxxStreamBuff::skip_space() {
  for (auto avail = in_avail(); avail > 0; --avail) {
    if (!std::isspace(sgetc())) {
      break;
    }
    sbumpc();
  }
}

void ReplxxStreamBuff::start() {
  skip_space();
  if (in_avail() > 0) {
    std::string new_buff(gptr(), egptr() - gptr());
    buffer_ = new_buff;
  } else {
    buffer_ = readline(PS1);
    buffer_ += "\n";
  }
  setg(buffer_.data(), buffer_.data(), buffer_.data() + buffer_.length());
}
