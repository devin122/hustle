/*
 * Copyright (c) 2023, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef HUSTLE_VM_CASTING_HPP
#define HUSTLE_VM_CASTING_HPP

#include "hustle/Core.hpp"
#include "hustle/Support/Utility.hpp"

#include <stdlib.h>
namespace hustle {

namespace cellcastimpl {

// hack to recreate Object::tag(), to avoid dependency issues
inline ObjectTag get_object_tag(Object* obj) {
  uintptr_t header = *(uintptr_t*)obj;
  return (ObjectTag)get_bits<OBJECT_TAG_BITS + 1, 1>(header);
}
template <typename T>
struct CellCastHelper {
  using CastType = T*;
  static bool is_a(cell_t cell) {
    if (cell_helpers::get_cell_type(cell) == TagHelper<T>::cell_tag) {
      Object* obj = cell_helpers::get_cell_pointer(cell);
      if (obj != nullptr) {
        return get_object_tag(obj) == TagHelper<T>::object_tag;
      } else {
        // TODO: this is here to get compiliation working. Going forward IDK if
        // we want these semantics
        return true;
      }
    }
    return false;
  }
  static CastType cast(cell_t cell) {
    return (CastType)(cell_helpers::get_cell_pointer(cell));
  }
};

template <>
struct CellCastHelper<intptr_t> {
  using CastType = intptr_t;
  static bool is_a(cell_t cell) {
    return cell_helpers::get_cell_type(cell) == TagHelper<intptr_t>::cell_tag;
  }
  static CastType cast(cell_t cell) { return cell_helpers::get_cell_int(cell); }
};
} // namespace cellcastimpl

// can we make this constexpr
template <typename T>
inline bool is_a(cell_t cell) {
  return cellcastimpl::CellCastHelper<T>::is_a(cell);
}

template <>
inline constexpr bool is_a<Object>(cell_t cell) {
  return cell_helpers::is_cell_on_heap(cell);
}

template <typename T>
inline typename cellcastimpl::CellCastHelper<T>::CastType cast(cell_t cell) {
  HSTL_ASSERT(is_a<T>(cell));
  return cellcastimpl::CellCastHelper<T>::cast(cell);
}

template <typename T>
auto dispatch_object(Object* obj, T fn) {
  HSTL_ASSERT(obj != nullptr);
  switch (cellcastimpl::get_object_tag(obj)) {
  case OBJ_TAG_ARRAY:
    return fn((Array*)obj);
  case OBJ_TAG_STRING:
    return fn((String*)obj);
  case OBJ_TAG_QUOTE:
    return fn((Quotation*)obj);
  case OBJ_TAG_RECORD:
    return fn((Record*)obj);
  case OBJ_TAG_WORD:
    return fn((Word*)obj);
  case OBJ_TAG_WRAPPER:
    return fn((Wrapper*)obj);
  default:
    abort(); // TODO better error handling
  }
}

template <typename T>
auto dispatch_cell(cell_t cell, T fn) {
  switch (cell_helpers::get_cell_type(cell)) {
  case CELL_TAG_INT:
    return fn(cell_helpers::get_cell_int(cell));
  case CELL_TAG_OBJ: {
    return dispatch_object(cell_helpers::get_cell_pointer(cell), fn);
  }
  default:
    abort(); // TODO better error handling
  }
}

} // namespace hustle

#endif