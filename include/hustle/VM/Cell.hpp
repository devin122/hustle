/*
 * Copyright (c) 2023, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef HUSTLE_VM_CELL_HPP
#define HUSTLE_VM_CELL_HPP

#include <hustle/Core.hpp>
#include <hustle/Support/Assert.hpp>
#include <hustle/VM/Casting.hpp>
#include <type_traits>

namespace hustle {
struct Object;

// casting stuff

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

  template <typename T>
  Cell(T* ptr) : raw_(cell_helpers::make_cell(ptr)) {}

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
    return Cell(cell_helpers::make_cell(intval));
  }
  static constexpr Cell from_raw(cell_t cell) noexcept { return Cell(cell); }

  /**
   * Dummy function
   *
   * This is a bogus function to aid in migrating code from cell_t to Cell
   */
  [[deprecated]] static constexpr Cell from_raw(Cell c) noexcept { return c; }

  // constexpr bool is_immediate() const { return tag() == CELL_TAG_INT; }
  constexpr bool is_int() const { return tag() == CELL_TAG_INT; }
  constexpr bool is_object() const { return tag() == CELL_TAG_OBJ; }

  constexpr intptr_t get_int() const {
    HSTL_ASSERT(is_int());
    return cell_helpers::get_cell_int(raw_);
  }

  Object* get_object() const {
    HSTL_ASSERT(is_object());
    return cell_helpers::get_cell_pointer(raw_);
  }
  /*


  */

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
  constexpr CellTag tag() const noexcept {
    return cell_helpers::get_cell_type(raw_);
  }

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

// Hack for migrating code
template <typename T>
auto cast(Cell c) {
  return c.cast<T>();
}

template <typename T>
bool is_a(Cell c) {
  return c.is_a<T>();
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
  constexpr TypedCell() : Cell(CELL_TAG_OBJ) {}
  TypedCell(T* ptr) : Cell(cell_helpers::make_cell(ptr)) {}

  operator T*() const noexcept {
    // HSTL_ASSERT(is_a<T>());
    // Object* obj = get_object();
    // HSTL_ASSERT(obj->tag() == T::TAG_VALUE);
    return hustle::cast<T>(raw_);
  }

  T* operator->() { return *this; }
  TypedCell& operator=(T* ptr) noexcept {
    raw_ = cell_helpers::make_cell(ptr);
    return *this;
  }

  TypedCell& operator=(Cell c) {
    HSTL_ASSERT(c.is_object());
    Object* obj = c.get_object();

    if (obj != nullptr) {
      HSTL_ASSERT(c.is_a<T>());
    }
    Cell::operator=(c);
    return *this;
  }
  // operator Cell() { return Cell(raw_); }
  // constexpr operator cell_t() noexcept { return raw_; }
} HUSTLE_HEAP_POINTER;

}; // namespace hustle
#endif
