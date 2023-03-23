/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef HUSTLE_CELL_HPP
#define HUSTLE_CELL_HPP

#include <hustle/Core.hpp>
#include <hustle/Support/Assert.hpp>
#include <type_traits>

namespace hustle {
struct Object;
inline constexpr cell_tag get_cell_type(cell_t c) {
  return (cell_tag)(c & CELL_TAG_MASK);
}

inline Object* get_cell_pointer(cell_t c) {
  HSTL_ASSERT(get_cell_type(c) != CELL_INT);
  return (Object*)(c & (~CELL_TAG_MASK));
}

inline intptr_t get_cell_int(cell_t c) {
  HSTL_ASSERT(get_cell_type(c) == CELL_INT);
  return ((intptr_t)c) >> CELL_TAG_BITS;
}

inline constexpr bool is_cell_on_heap(cell_t c) {
  return get_cell_type(c) != CELL_INT;
}

inline constexpr cell_t make_cell(Object* ptr, cell_tag tag) {
  // HSTL_ASSERT(((uintptr_t)ptr & CELL_TAG_MASK) == 0);
  return (uintptr_t)ptr | tag;
}

template <typename T>
cell_t make_cell(T* ptr) {
  static_assert(std::is_base_of_v<Object, T>,
                "Attempted to make cell from non-object");
  return make_cell(ptr, T::TAG_VALUE);
}

template <typename T>
constexpr cell_t make_cell(std::nullptr_t) {
  return T::TAG_VALUE;
}

inline constexpr cell_t make_cell(intptr_t value) {
  // TODO we should probably assert we arent loosing any precision
  // constexpr uintptr_t value_mask = ~(UINTPTR_MAX >> CELL_TAG_BITS);
  // assert((value & value_mask) == 0);
  return (cell_t)(((uintptr_t)value) << CELL_TAG_BITS);
}

namespace cellcastimpl {

template <typename T>
struct TagHelper {
  static constexpr cell_tag value = T::TAG_VALUE;
};

template <>
struct TagHelper<intptr_t> {
  static constexpr cell_tag value = CELL_INT;
};

template <typename T>
static constexpr cell_tag tag_value = TagHelper<T>::value;

template <typename T>
struct CellCastHelper {
  using CastType = T*;
  static CastType cast(cell_t cell) {
    return (CastType)(cell & ~CELL_TAG_MASK);
  }
};

template <>
struct CellCastHelper<intptr_t> {
  using CastType = intptr_t;
  static CastType cast(cell_t cell) { return get_cell_int(cell); }
};
} // namespace cellcastimpl

template <typename T>
inline constexpr bool is_a(cell_t cell) {
  return get_cell_type(cell) == cellcastimpl::tag_value<T>;
}

template <>
inline constexpr bool is_a<Object>(cell_t cell) {
  return is_cell_on_heap(cell);
}

template <typename T>
inline typename cellcastimpl::CellCastHelper<T>::CastType cast(cell_t cell) {
  HSTL_ASSERT(is_a<T>(cell));
  return cellcastimpl::CellCastHelper<T>::cast(cell);
}

/***
 * Main datatype in hustle.
 *
 * Represent a single cell of data. Essentially acts as a strongly typed alias
 * for cell_t.
 */
class Cell {
public:
  /**
   * Create Cell with value (int)0.
   */
  constexpr Cell() = default;

  /**
   * Obtain the "raw" value of this cell.
   *
   * This should probably be avoided in most code.
   */
  constexpr cell_t raw() const noexcept { return raw_; } // gross

  /**
   * Set the raw value of this cell.
   *
   * This should probably be avoided in most code.
   */
  constexpr void set_raw(cell_t value) noexcept { raw_ = value; }

  /**
   * Create a Cell representing a given integer value
   */
  static constexpr Cell from_int(intptr_t intval) noexcept {
    return Cell(make_cell(intval));
  }
  static constexpr Cell from_raw(cell_t cell) noexcept { return Cell(cell); }

  /**
   * Dummy function
   *
   * This is a bogus function to aid in migrating code from cell_t to Cell
   */
  [[deprecated]] static constexpr Cell from_raw(Cell c) noexcept { return c; }

  template <typename T>
  Cell(T* ptr) : raw_(make_cell(ptr)) {}

  template <typename T>
  auto cast() {
    return hustle::cast<T>(raw_);
  }

  template <typename T>
  constexpr bool is_a() const {
    return ::hustle::is_a<T>(raw_);
  }

  /**
   * Identity comparison.
   *
   * \note also checks equivilence of type, even for nullptr. So
   * (String*)nullptr != (Array*)nullptr
   */
  constexpr bool operator==(const Cell& other) const noexcept {
    return other.raw_ == raw_;
  }

  /**
   * \sa operator==
   */
  constexpr bool operator!=(const Cell& other) const noexcept {
    return other.raw_ != raw_;
  }

  /**
   * Get the tag value of this Cell.
   */
  constexpr cell_tag tag() const noexcept { return get_cell_type(raw_); }

  /*
    constexpr Cell operator=(const Cell other) noexcept {
      raw_ = other.raw_;
      return *this;
    }
  */
protected:
  explicit constexpr Cell(cell_t cell) noexcept : raw_(cell) {}
  cell_t raw_ = 0;
} HUSTLE_HEAP_POINTER;

// constexpr TypedCell<intptr_t> operator"" _c(intptr_t x) { return
// Cell::from_int(x); }
static_assert(sizeof(Cell) == sizeof(cell_t));

template <typename T>
inline typename cellcastimpl::CellCastHelper<T>::CastType cast(Cell cell) {
  HSTL_ASSERT(cell.is_a<T>());
  return cellcastimpl::CellCastHelper<T>::cast(cell.raw());
}

template <typename T>
inline constexpr bool is_a(Cell cell) {
  return is_a<T>(cell.raw());
}

/**
 * Type checked Cell.
 *
 * Behaves like a Cell, but assume it holds a value of a given type. For types
 * other than intptr_t, this objects of this class can be treated as a pointer
 * to the type.
 *
 * \todo should be asserting that this class lays out as we would expect.
 */
template <typename T>
class TypedCell : public Cell {
public:
  constexpr TypedCell() : Cell(T::TAG_VALUE) {}
  TypedCell(T* ptr) : Cell(make_cell<T>(ptr)) {}

  operator T*() const noexcept {
    HSTL_ASSERT(is_a<T>());
    return (T*)get_cell_pointer(raw_);
  }

  T* operator->() { return *this; }
  TypedCell& operator=(T* ptr) noexcept {
    raw_ = make_cell(ptr);
    return *this;
  }

  TypedCell& operator=(Cell c) {
    HSTL_ASSERT(c.is_a<T>());
    Cell::operator=(c);
    return *this;
  }
  // operator Cell() { return Cell(raw_); }
  // constexpr operator cell_t() noexcept { return raw_; }
} HUSTLE_HEAP_POINTER;

} // namespace hustle

#endif
