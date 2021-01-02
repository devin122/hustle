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

/**
 * \file
 * miscelaneous support utilities
 */

#ifndef HUSTLE_SUPPORT_UTILITY_HPP
#define HUSTLE_SUPPORT_UTILITY_HPP

#include <filesystem>
namespace hustle {

/**
 * Save the value of argv0.
 *
 * This should be called in main() at the start of the program, before changing
 * the working directory. Calling this function is required for
 * hustle_lib_dir() and get_exe_path() to work.
 *
 */
void save_argv0(const char* argv0);

/**
 * Get the path to the the hustle std library files.
 *
 * \note requires call to save_argv0() first.
 */
std::filesystem::path hustle_lib_dir();

/**
 * Get the path to the currently running executable.
 *
 * \note requires call to save_argv0() first.
 */
std::filesystem::path get_exe_path();

/**
 * Check if a given value is a power of 2
 */
template <typename T>
inline constexpr bool is_power_of_2(T val) {
  return 0 == ((val - 1) & val);
}

/**
 * Extract given bits from a value.
 *
 * \tparam high,low range of bits to extract.
 */
template <uint8_t high, uint8_t low = high>
inline constexpr uintptr_t get_bits(uintptr_t raw) {
  constexpr uint8_t bits = sizeof(raw) * 8;
  static_assert(high < bits,
                "Provided high param outside the number of bits in input");
  static_assert(low <= high, "Low param must be <= the high param");
  constexpr uintptr_t mask =
      (static_cast<uintptr_t>(-1)) >> ((bits - 1) - high);
  return (raw & mask) >> low;
}

/**
 * Set the given bits in a value
 *
 * \tparam high,low range of bits to set.
 */
template <uint8_t high, uint8_t low = high>
inline constexpr uintptr_t set_bits(uintptr_t value, uintptr_t existing = 0) {
  constexpr uint8_t bits = sizeof(existing) * 8;
  static_assert(high < bits,
                "Provided high param outside the number of bits in input");
  static_assert(low <= high, "Low param must be <= the high param");

  constexpr uintptr_t high_mask = ((uintptr_t)(-1)) >> ((bits - 1) - high);
  constexpr uintptr_t low_mask = ((1 << low) - 1);
  constexpr uintptr_t mask = high_mask & ~low_mask;

  return (existing & ~mask) | ((value << low) & mask);
}

/**
 * Add an offet to a pointer.
 *
 * \todo figuure out a way to make T default to F
 */
template <typename T, typename F>
inline T* pointer_add(F* ptr, uintptr_t offset) {
  uint8_t* rc = (uint8_t*)ptr;
  return (T*)(rc + offset);
}

struct VM;
void dump_stack(const VM& vm, bool recurse = true);
void print_stack(const VM& vm, unsigned max_depth = -1);

} // namespace hustle

#endif
