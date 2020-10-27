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
