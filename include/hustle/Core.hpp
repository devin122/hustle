/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * \file
 * Core definitions for the language
 */

#ifndef HUSTLE_CORE_HPP
#define HUSTLE_CORE_HPP

#include "hustle/Support/Assert.hpp"
#include <stdint.h>

namespace hustle {

/**
 * Low level definition of our core Cell data type.
 *
 * This should only be used by low level code.
 * Most code should use  \ref Cell instead.
 *
 * \todo This should be moved into a private namespace at some point
 */
using cell_t = uintptr_t;

enum CellTag { CELL_TAG_INT, CELL_TAG_OBJ, CELL_TAG_MAX };

/// Number of bits used for pointer tagging
constexpr uintptr_t CELL_TAG_BITS = 3;

/// Mask value to get the pointer tag
constexpr uintptr_t CELL_TAG_MASK = (1 << CELL_TAG_BITS) - 1;
static_assert(CELL_TAG_MAX <= (1 << CELL_TAG_BITS), "Too many cell tag types");

// Limits on the magnitude of cell_t used to store int
constexpr intptr_t CELL_INT_MIN = INTPTR_MIN >> CELL_TAG_BITS;
constexpr intptr_t CELL_INT_MAX = INTPTR_MAX >> CELL_TAG_BITS;

// Basic sanity checks of our min/max values
static_assert(CELL_INT_MIN < 0);
static_assert(CELL_INT_MAX > 0);

enum ObjectTag : uint8_t {
  OBJ_TAG_ARRAY,
  OBJ_TAG_STRING,
  OBJ_TAG_QUOTE,
  OBJ_TAG_RECORD,
  OBJ_TAG_WORD,
  OBJ_TAG_WRAPPER,
  OBJ_TAG_MAX
};
inline const char* get_object_name(ObjectTag t) {
  switch (t) {
  case OBJ_TAG_ARRAY:
    return "Array";
  case OBJ_TAG_STRING:
    return "String";
  case OBJ_TAG_QUOTE:
    return "Quote";
  case OBJ_TAG_RECORD:
    return "Record";
  case OBJ_TAG_WORD:
    return "Word";
  case OBJ_TAG_WRAPPER:
    return "Wrapper";
  default:
    HSTL_ASSERT(false);
  }
}
constexpr uint8_t OBJECT_TAG_BITS = 8;
constexpr uintptr_t OBJECT_TAG_MASK = (1 << OBJECT_TAG_BITS) - 1;

struct Object;
struct Array;
struct String;
struct Quotation;
struct Record;
struct Word;
struct Wrapper;

template <typename T>
struct TagHelper {};

template <>
struct TagHelper<intptr_t> {
  static constexpr CellTag cell_tag = CELL_TAG_INT;
};

template <>
struct TagHelper<Object> {
  static constexpr CellTag cell_tag = CELL_TAG_OBJ;
};

template <>
struct TagHelper<Array> : public TagHelper<Object> {
  static constexpr ObjectTag object_tag = OBJ_TAG_ARRAY;
};

template <>
struct TagHelper<String> : public TagHelper<Object> {
  static constexpr ObjectTag object_tag = OBJ_TAG_STRING;
};

template <>
struct TagHelper<Quotation> : public TagHelper<Object> {
  static constexpr ObjectTag object_tag = OBJ_TAG_QUOTE;
};

template <>
struct TagHelper<Record> : public TagHelper<Object> {
  static constexpr ObjectTag object_tag = OBJ_TAG_RECORD;
};

template <>
struct TagHelper<Word> : public TagHelper<Object> {
  static constexpr ObjectTag object_tag = OBJ_TAG_WORD;
};

template <>
struct TagHelper<Wrapper> : public TagHelper<Object> {
  static constexpr ObjectTag object_tag = OBJ_TAG_WRAPPER;
};

/// Helper functions for cell_t values
namespace cell_helpers {

inline constexpr CellTag get_cell_type(cell_t c) {
  return (CellTag)(c & CELL_TAG_MASK);
}

inline Object* get_cell_pointer(cell_t c) {
  HSTL_ASSERT(get_cell_type(c) != CELL_TAG_INT);
  return (Object*)(c & (~CELL_TAG_MASK));
}

inline constexpr intptr_t get_cell_int(cell_t c) {
  HSTL_ASSERT(get_cell_type(c) == CELL_TAG_INT);
  return ((intptr_t)c) >> CELL_TAG_BITS;
}

inline constexpr bool is_cell_on_heap(cell_t c) {
  return get_cell_type(c) != CELL_TAG_INT;
}

inline constexpr cell_t make_cell(Object* ptr, CellTag tag) {
  // HSTL_ASSERT(((uintptr_t)ptr & CELL_TAG_MASK) == 0);
  return (uintptr_t)ptr | tag;
}

inline constexpr cell_t make_cell(Object* ptr) {
  return make_cell(ptr, CELL_TAG_OBJ);
}

// constexpr cell_t make_cell(Object* obj) { return T::TAG_VALUE; }

inline constexpr cell_t make_cell(intptr_t value) {
  // TODO we should probably assert we arent loosing any precision
  // constexpr uintptr_t value_mask = ~(UINTPTR_MAX >> CELL_TAG_BITS);
  // assert((value & value_mask) == 0);
  return (cell_t)((((uintptr_t)value) << CELL_TAG_BITS) | CELL_TAG_INT);
}

} // namespace cell_helpers

} // namespace hustle

// GC safety annotations.
#if defined(HUSTLE_GC_SAFETY)
#define GC_SAFETY(x) __attribute((annotate("gc::" #x)))
#else
#define GC_SAFETY(x)
#endif

/// Mark a function as pontentially triggering a GC
#define HUSTLE_MAY_ALLOCATE GC_SAFETY(may_allocate)

/// Mark a class as being allocated on the GC'd heap
#define HUSTLE_HEAP_ALLOCATED GC_SAFETY(heap_allocated)

/// Marks a class as aliasing a heap pointer
#define HUSTLE_HEAP_POINTER GC_SAFETY(heap_pointer)

#endif
