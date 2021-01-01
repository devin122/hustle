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

#ifndef HUSTLE_OBJECT_HPP
#define HUSTLE_OBJECT_HPP

#include "Core.hpp"
#include "cell.hpp"
#include "hustle/Utility.hpp"
#include <algorithm>
#include <assert.h>
#include <atomic>
#include <gsl/gsl>
#include <initializer_list>

namespace hustle {
struct VM;

template <typename T, typename... Us>
static size_t object_allocation_size(Us...) {
  return sizeof(T);
}

// TODO: split concept of object resolution from min object size
constexpr size_t OBJECT_RESOLUTION = 8;
static_assert(is_power_of_2(OBJECT_RESOLUTION),
              "Object resolution needs to be power of 2");
static_assert((1 << CELL_TAG_BITS) <= OBJECT_RESOLUTION,
              "Not enough bits for pointer resolution with given tag bits");
struct Object {
  // 0: forwarding bits;
  // 63:1 - forwarding ptr (if forwarding bit set)
  // TAG_BITS+1:1 - tag
  // 63:TAG_BITS+2 - size;
  Object(cell_tag tag, size_t size) noexcept {
    uintptr_t tmp_header = set_bits<CELL_TAG_BITS + 1, 1>(tag);
    header = set_bits<63, CELL_TAG_BITS + 2>(size, tmp_header);
  }
  // constexpr Object(uint32_t head = 0, uint32_t sz = sizeof(Object)) noexcept:
  // header(head), size_(sz) {}
  template <typename T>
  constexpr Object(T* dummy, size_t extra = 0) noexcept
      : Object(T::TAG_VALUE, sizeof(T) + extra) {}
  constexpr uint32_t size() const {
    return gsl::narrow_cast<uint32_t>(get_bits<63, CELL_TAG_BITS + 2>(header));
  }
  constexpr cell_tag tag() const noexcept {
    return (cell_tag)get_bits<CELL_TAG_BITS + 1, 1>(header);
  }

  Object* next_object() const noexcept {
    uint8_t* raw = (uint8_t*)this;
    raw += size();
    return (Object*)this;
  }

  bool is_forwarding() const noexcept { return get_bits<0, 0>(header); }
  Object* get_forwarding() const noexcept {
    HSTL_ASSERT(is_forwarding());
    return (Object*)(header & ~1);
  }

  void forward_to(Object* obj) {
    uintptr_t obj_raw = (uintptr_t)obj;
    HSTL_ASSERT((obj_raw & 1) == 0);
    header = obj_raw | 1;
  }

  Cell get_cell() const {
    HSTL_ASSERT(!is_forwarding());
    uintptr_t raw_ptr = (uintptr_t)this;
    HSTL_ASSERT((raw_ptr & CELL_TAG_MASK) == 0);
    return Cell::from_raw(raw_ptr | (CELL_TAG_MASK & tag()));
  }

private:
  uintptr_t header = 0;

  // constexpr bool is_marked() const noexcept { return header.marked; }
  // constexpr void mark(bool m = true) noexcept { header.marked = m ? 1: 0;}
} HUSTLE_HEAP_ALLOCATED;

#include "classes.def"

inline Array::Array(std::initializer_list<Cell> init) noexcept
    : Object(this, init.size() * sizeof(Cell)) {
  std::copy(init.begin(), init.end(), data());
}

constexpr Array::Array(size_t ct) noexcept
    : Object(this, sizeof(cell_t) * ct) {}
inline size_t Array::count() const {
  HSTL_ASSERT((size() - sizeof(Array)) % sizeof(Cell) == 0);
  return (size() - sizeof(Array)) / sizeof(Cell);
}

inline Array* Array::from_list(std::initializer_list<Cell> init) {
  Array* arr = Array::create(init.size());
  std::copy(init.begin(), init.end(), arr->data());
  return arr;
}

inline Cell& Array::operator[](size_t idx) {
  HSTL_ASSERT(idx < count());
  return data()[idx];
}

inline Record::Record(size_t count) noexcept
    : Object(this, sizeof(cell_t) * count) {
  // TODO initialize slots?
}

template <>
[[maybe_unused]] size_t object_allocation_size<Array>(size_t s) {
  return sizeof(cell_t) * s + sizeof(Array);
}

template <>
[[maybe_unused]] size_t
object_allocation_size<Array>(std::initializer_list<cell_t> init) {
  return init.size() * sizeof(cell_t) + sizeof(Array);
}

/*
inline String::String(const char* c_str) : Object(this, strlen(c_str)+1),
length(strlen(c_str)) { std::copy(c_str, c_str + length + 1, data());
};
*/
inline String::String(const char* c_str, size_t len) noexcept
    : Object(this, len + 1), length_raw(Cell::from_int(len)) {
  std::copy(c_str, c_str + len, data());
  data()[len] = 0;
}

// TODO this should use a delegating constructor, but then we have issue with
// calling a deleted destructor
inline String::String(std::string_view sv) noexcept
    : Object(this, sv.size() + 1), length_raw(Cell::from_int(sv.size())) {
  std::copy(sv.cbegin(), sv.cend(), data());
  data()[sv.size()] = 0;
}

inline size_t String::length() const { return cast<intptr_t>(length_raw); }

template <>
[[maybe_unused]] size_t object_allocation_size<String>(size_t sz) {
  return sz + sizeof(String);
}

template <>
[[maybe_unused]] size_t object_allocation_size<String>(int64_t sz) {
  return sz + sizeof(String);
}

template <>
[[maybe_unused]] size_t object_allocation_size<String>(const char*, size_t sz) {
  return sz + sizeof(String) + 1;
}

template <>
[[maybe_unused]] size_t object_allocation_size<String>(const char*,
                                                       int64_t sz) {
  return sz + sizeof(String) + 1;
}

inline gsl::string_span<gsl::dynamic_extent> String::to_span() {
  return {data(), length()};
}

inline String::operator std::string_view() const { return {data(), length()}; }
static_assert(sizeof(Object) == sizeof(uintptr_t),
              "Object size should be size of header");

} // namespace hustle
#endif
