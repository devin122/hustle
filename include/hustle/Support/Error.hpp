/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*
 * \file
 * Misc functionality for dealing with various errors
 */

#ifndef HUSTLE_SUPPORT_ERROR_HPP
#define HUSTLE_SUPPORT_ERROR_HPP

#include <exception>
#include <variant>
namespace hustle {

class Exception : public std::exception {
public:
  Exception(const char* msg) : msg_(msg) {}
  const char* what() const noexcept override { return msg_; }

private:
  const char* msg_;
};

class EndOfStreamException : public Exception {
public:
  using Exception::Exception;
};

} // namespace hustle
#endif
