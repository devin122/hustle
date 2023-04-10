/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef HUSTLE_OBJECT_HPP
#define HUSTLE_OBJECT_HPP

#include "Core.hpp"
#include "hustle/Support/Assert.hpp"
#include "hustle/Support/Utility.hpp"
#include "hustle/VM/Cell.hpp"
#include <algorithm>
#include <assert.h>
#include <atomic>
#include <gsl/gsl>
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
  // 9:1 - tag
  // 63:10 - size;
  Object(ObjectTag tag, size_t size) noexcept {
    uintptr_t tmp_header = set_bits<OBJECT_TAG_BITS + 1, 1>(tag);
    header = set_bits<63, 10>(size, tmp_header);
  }
  // constexpr Object(uint32_t head = 0, uint32_t sz = sizeof(Object)) noexcept:
  // header(head), size_(sz) {}
  template <typename T>
  constexpr Object(T* dummy, size_t extra = 0) noexcept
      : Object(TagHelper<T>::object_tag, sizeof(T) + extra) {}
  constexpr uint32_t size() const {
    return gsl::narrow_cast<uint32_t>(
        get_bits<63, OBJECT_TAG_BITS + 2>(header));
  }
  constexpr ObjectTag tag() const noexcept {
    return (ObjectTag)get_bits<OBJECT_TAG_BITS + 1, 1>(header);
  }

  Object* next_object() const noexcept {
    uint8_t* raw = (uint8_t*)this;
    raw += size();
    return (Object*)raw;
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
    // TODO: we need to handle the case of some objects having a tag
    // HSTL_ASSERT((raw_ptr & OBJECT_TAG_MASK) == 0);
    return Cell(const_cast<Object*>(this));
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

// todo: doesnt really belong here
inline const char* get_type_name(Cell cell) {
  if (cell.is_int()) {
    return "int";
  } else {
    Object* obj = cell.get_object();
    HSTL_ASSERT(obj != nullptr);
    return get_object_name(obj->tag());
  }
}

/**
 * Wrap another object to prevent execution.
 *
 * When the interpreter encounters a wrapper object during execution, it pushes
 * the wrapped value on the stack. This is useful when you want to use a value
 * which would otherwise be executed (such as a Word)
 *
 */
struct Wrapper : public Object {
  static constexpr ObjectTag TAG_VALUE = OBJ_TAG_WRAPPER;

  Wrapper() : Object(this) {}
  ~Wrapper() = delete;

  Cell wrapped;
} HUSTLE_HEAP_ALLOCATED;

/**
 * Fixed size array of Cell values.
 *
 * \note Since we rely on the Object size, this class only works if the object
 * resolution is the same as the Cell size
 */
struct Array : public Object {
  static constexpr ObjectTag TAG_VALUE = OBJ_TAG_ARRAY;

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

/**
 * \related Array
 */
inline size_t object_allocation_size(Array*, size_t s) {
  return sizeof(cell_t) * s + sizeof(Array);
}

/**
 * \related Array
 */
inline size_t object_allocation_size(Array*,
                                     std::initializer_list<cell_t> init) {
  return init.size() * sizeof(cell_t) + sizeof(Array);
}

/**
 * Semi-fixed length string value.
 *
 * The maximum size that a String can hold is dictated by the initial object
 * allocation size, and remains constant through the strings life.
 */
struct String : public Object {
  static constexpr ObjectTag TAG_VALUE = OBJ_TAG_STRING;
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

  /**
   * Get a pointer to the stored string value.
   *
   * \warning The raw string is not guaranteed to be null terminated.
   * Ensure that you limit access to lenght() characters
   */
  char* data() const { return pointer_add<char>(this, sizeof(String)); }

  /**
   * Get the maximum size which can be stored in this string.
   */
  size_t capacity() const;

  /**
   * Get the current length of the stored string
   */
  size_t length() const { return length_raw.get_int(); }

  operator std::string_view() const { return {data(), length()}; }

  /// Current length of the string
  Cell length_raw;
} HUSTLE_HEAP_ALLOCATED;

/**
 * \related String
 */
inline size_t object_allocation_size(String*, size_t sz) {
  return sz + sizeof(String);
}

/**
 * \related String
 */
inline size_t object_allocation_size(String*, const char*, size_t sz) {
  return sz + sizeof(String) + 1;
}

/**
 * Basic "code" block of the language.
 *
 * A Quotation is essentially a lambda.
 */
struct Quotation : public Object {
  static constexpr ObjectTag TAG_VALUE = OBJ_TAG_QUOTE;
  ~Quotation() = delete;
  using FuncType = void (*)(VM*, Quotation*);
  Quotation() : Object(this){};

  /**
   * Create a quote for a given primitive function
   */
  Quotation(FuncType primitive)
      : Object(this), definition(), entry(primitive) {}
  Quotation(Array* def, FuncType ent)
      : Object(this), definition(def), entry(ent) {}

  /**
   * The array used to define this Quotation
   *
   * \note This value will be null for primitives
   */
  TypedCell<Array> definition;

  /**
   * An optional native entry point for this quote.
   *
   * Currently this is only provided for primitives, however this would
   * also be set once we start jit compiling quotes
   */
  FuncType entry;
} HUSTLE_HEAP_ALLOCATED;

/**
 * \related Quotation
 */
inline size_t object_allocation_size(Quotation*, Quotation::FuncType) {
  return sizeof(Quotation);
}

struct Record : public Object {
  static constexpr ObjectTag TAG_VALUE = OBJ_TAG_RECORD;
  ~Record() = delete;
  Record(size_t count) noexcept : Object(this, sizeof(cell_t) * count) {
    // TODO initialize slots?
  }

  cell_t slots[];
} HUSTLE_HEAP_ALLOCATED;

/**
 * A Word is a named Quotation.
 */
struct Word : public Object {
  static constexpr ObjectTag TAG_VALUE = OBJ_TAG_WORD;
  ~Word() = delete;
  Word() : Object(this){};
  TypedCell<String> name;
  TypedCell<Quotation> definition;
  Cell properties; // Hash table

  /**
   * Indicate if this is a parseword (eg if it is evaluated immediately at parse
   * time).
   *
   * \todo this should be stored in the properties hash table when it is
   * implemented
   */
  bool is_parse_word = false;
} HUSTLE_HEAP_ALLOCATED;

//#include "classes.def"

static_assert(sizeof(Object) == sizeof(uintptr_t),
              "Object size should be size of header");

} // namespace hustle
#endif
