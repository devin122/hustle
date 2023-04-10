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

#include <hustle/Support/Assert.hpp>

#include <concepts>
#include <cstddef>
#include <exception>
#include <type_traits>
#include <utility>
#include <variant>
namespace hustle {

// Draws heavily on llvm::Expected
template <typename T>
class [[nodiscard]] Expected {
  // hack so I can tweak the error type late
  static constexpr bool is_ref = std::is_reference<T>::value;

  using wrap = std::reference_wrapper<std::remove_reference_t<T>>;

  using error_type = int; // std::unique_ptr<ErrorInfoBase>;

public:
  using storage_type = std::conditional_t<is_ref, wrap, T>;
  using value_type = T;

private:
  using reference = std::remove_reference_t<T>&;
  using const_reference = const std::remove_reference_t<T>&;
  using pointer = std::remove_reference_t<T>*;

public:
  Expected(error_type err) : has_error_(true) {
    // TODO: this may need hacking when error_type is not int
    *get_error_storage() = err;
  }
#if 0
  template <std::convertible_to<T> T2>
  Expected(T2&& v) {
    new (get_storage()) storage_type(std::forward<T2>(v));
  }
#else

  Expected(T&& v) { new (get_storage()) storage_type(std::forward<T>(v)); }
#endif

  // TODO
  Expected(Expected&&) = delete;

  reference get() { return *get_storage(); }

  explicit operator bool() { return !has_error_; }

  error_type error() { return *get_error_storage(); }

private:
  error_type* get_error_storage() {
    HSTL_ASSERT(has_error_);
    return reinterpret_cast<error_type*>(&error_storage_);
  }

  storage_type* get_storage() {
    HSTL_ASSERT(!has_error_);
    return reinterpret_cast<storage_type*>(&value_storage_);
  }
  union {
    alignas(storage_type) std::byte value_storage_[sizeof(storage_type)];
    alignas(error_type) std::byte error_storage_[sizeof(error_type)];
  };
  bool has_error_ : 1;
};

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
