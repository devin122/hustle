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
#include <algorithm>
#include <assert.h>
#include <atomic>
#include <gsl/gsl>
#include <hustle/Support/Utility.hpp>
#include <initializer_list>

namespace hustle {
struct VM;

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

// default object size
template <typename T>
inline size_t object_allocation_size(T*) {
  return sizeof(T);
}

struct Wrapper : public Object {
  static constexpr cell_tag TAG_VALUE = CELL_WRAPPER;

  Wrapper() : Object(this) {}
  ~Wrapper() = delete;

  Cell wrapped;
} HUSTLE_HEAP_ALLOCATED;

struct Array : public Object {
  static constexpr cell_tag TAG_VALUE = CELL_ARRAY;

  Array(std::initializer_list<Cell> init) noexcept
      : Object(this, init.size() * sizeof(Cell)) {
    std::copy(init.begin(), init.end(), data());
  }
  Array(size_t ct) noexcept : Object(this, sizeof(cell_t) * ct) {}

  ~Array() = delete;

  Cell* data() const noexcept { return (Cell*)(((char*)this) + sizeof(Array)); }
  static Array* create(size_t sz);

  inline size_t count() const {
    HSTL_ASSERT((size() - sizeof(Array)) % sizeof(Cell) == 0);
    return (size() - sizeof(Array)) / sizeof(Cell);
  }

  Cell* begin() const { return data(); }
  Cell* end() const { return data() + count(); }

  Cell& operator[](size_t idx) {
    HSTL_ASSERT(idx < count());
    return data()[idx];
  }
} HUSTLE_HEAP_ALLOCATED;
inline size_t object_allocation_size(Array*, size_t s) {
  return sizeof(cell_t) * s + sizeof(Array);
}

inline size_t object_allocation_size(Array*,
                                     std::initializer_list<cell_t> init) {
  return init.size() * sizeof(cell_t) + sizeof(Array);
}

struct String : public Object {
  static constexpr cell_tag TAG_VALUE = CELL_STRING;
  ~String() = delete;
  String(size_t len) : Object(this, len), length_raw(Cell::from_int(0)) {}
  String(const char* c_str, size_t len) noexcept
      : Object(this, len + 1), length_raw(Cell::from_int(len)) {
    std::copy(c_str, c_str + len, data());
    data()[len] = 0;
  }
  // TODO this should use a delegating constructor, but then we have issue with
  // calling a deleted destructor
  String(std::string_view sv) noexcept
      : Object(this, sv.size() + 1), length_raw(Cell::from_int(sv.size())) {
    std::copy(sv.cbegin(), sv.cend(), data());
    data()[sv.size()] = 0;
  }

  char* data() const { return pointer_add<char>(this, sizeof(String)); }
  size_t capacity() const;
  size_t length() const { return cast<intptr_t>(length_raw); }

  gsl::string_span<gsl::dynamic_extent> to_span() { return {data(), length()}; }
  operator std::string_view() const { return {data(), length()}; }

  Cell length_raw;
} HUSTLE_HEAP_ALLOCATED;

inline size_t object_allocation_size(String*, size_t sz) {
  return sz + sizeof(String);
}

inline size_t object_allocation_size(String*, const char*, size_t sz) {
  return sz + sizeof(String) + 1;
}

struct Quotation : public Object {
  static constexpr cell_tag TAG_VALUE = CELL_QUOTE;
  ~Quotation() = delete;
  using FuncType = void (*)(VM*, Quotation*);
  Quotation() : Object(this){};
  Quotation(FuncType primitive)
      : Object(this), definition(), entry(primitive) {}
  Quotation(Array* def, FuncType ent)
      : Object(this), definition(def), entry(ent) {}
  TypedCell<Array> definition;
  FuncType entry;
} HUSTLE_HEAP_ALLOCATED;

inline size_t object_allocation_size(Quotation*, Quotation::FuncType) {
  return sizeof(Quotation);
}

struct Record : public Object {
  static constexpr cell_tag TAG_VALUE = CELL_RECORD;
  ~Record() = delete;
  Record(size_t count) noexcept : Object(this, sizeof(cell_t) * count) {
    // TODO initialize slots?
  }

  cell_t slots[];
} HUSTLE_HEAP_ALLOCATED;

struct Word : public Object {
  static constexpr cell_tag TAG_VALUE = CELL_WORD;
  ~Word() = delete;
  Word() : Object(this){};
  TypedCell<String> name;
  TypedCell<Quotation> definition;
  Cell properties;            // Hash table
  bool is_parse_word = false; // hack until we get properties working properly
} HUSTLE_HEAP_ALLOCATED;

#include "classes.def"

static_assert(sizeof(Object) == sizeof(uintptr_t),
              "Object size should be size of header");

} // namespace hustle
#endif
