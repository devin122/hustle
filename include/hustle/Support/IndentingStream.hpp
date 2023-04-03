/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef HUSTLE_SUPPORT_INDENTING_STREAM
#define HUSTLE_SUPPORT_INDENTING_STREAM

#include <fmt/core.h>

#include <ostream>
#include <utility>

namespace hustle {

/**
 * Simple stream which manages indenting
 */
class IndentingStream {
public:
  IndentingStream(std::ostream& os) : out_(os) {}

  template <typename... Args>
  IndentingStream& writeln(fmt::format_string<Args...> format_str, Args&&... args) {
    // TODO this is a hack
    for (unsigned i = 0; i < indent_level_; ++i) {
      out_ << "  ";
    }
    out_ << fmt::format(std::move(format_str), std::forward<Args>(args)...) << "\n";
    return *this;
  }

  /**
   * Increase indent level of the stream
   *
   * \returns this stream
   */
  IndentingStream& indent() {
    ++indent_level_;
    return *this;
  }

  /**
   * Decrease indent level of the stream
   *
   * If indent level is already 0, do nothing
   * \returns this stream
   */
  IndentingStream& outdent() {
    if (indent_level_ != 0) {
      --indent_level_;
    }
    return *this;
  }

  /**
   * Write a new line to the stream.
   *
   * New line will not have any indentation
   * \returns this stream
   */
  IndentingStream& nl() {
    out_ << "\n";
    return *this;
  }

  /**
   * Get the current indentation level
   */
  constexpr unsigned depth() const noexcept { return indent_level_; }

private:
  unsigned indent_level_ = 0;
  std::ostream& out_;
};
} // namespace hustle
#endif
