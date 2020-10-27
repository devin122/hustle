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

#ifndef HUSTLE_SUPPORT_INDENTING_STREAM
#define HUSTLE_SUPPORT_INDENTING_STREAM

#include <fmt/core.h>
#include <ostream>

namespace hustle {
class IndentingStream {
public:
  IndentingStream(std::ostream& os) : out_(os) {}

  template <typename S, typename... Args>
  IndentingStream& writeln(const S& format_str, Args&&... args) {
    // TODO this is a hack
    for (unsigned i = 0; i < indent_level_; ++i) {
      out_ << "  ";
    }
    out_ << fmt::format(format_str, std::forward<Args>(args)...) << "\n";
    return *this;
  }

  IndentingStream& indent() {
    ++indent_level_;
    return *this;
  }
  IndentingStream& outdent() {
    if (indent_level_ != 0) {
      --indent_level_;
    }
    return *this;
  }
  IndentingStream& nl() {
    out_ << "\n";
    return *this;
  }

  constexpr unsigned depth() const noexcept { return indent_level_; }

private:
  unsigned indent_level_ = 0;
  std::ostream& out_;
};
} // namespace hustle
#endif
