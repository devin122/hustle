/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "hustle/Support/Error.hpp"
#include "hustle/VM.hpp"

#include <algorithm>
#include <new>
using namespace hustle;

Stack::Stack(size_t sz) {
  base_ = (Cell*)malloc(sizeof(Cell) * sz);
  if (base_ == nullptr) {
    throw std::bad_alloc();
  }
  top_ = base_ + sz;
  sp_ = top_;
  memset(base_, 0, sizeof(Cell) * sz);
}

Stack::~Stack() { free(base_); }

void Stack::clear() {
  std::fill(begin(), end(), Cell::from_int(0));
  sp_ = top_;
}

void Stack::push(Cell cell) {
  if (sp_ <= base_) {
    throw Exception("Stack overflow");
  }
  HSTL_ASSERT(sp_ <= top_);
  *--sp_ = cell;
}

Cell Stack::pop() {
  if (sp_ >= top_) {
    throw Exception("stack underflow");
  }
  HSTL_ASSERT(sp_ >= base_);
  Cell cell = *sp_;
  *sp_++ = Cell::from_raw(0);
  return cell;
}

Cell Stack::peek() const {
  if (sp_ >= top_) {
    throw Exception("stack underflow");
  }
  HSTL_ASSERT(sp_ >= base_);
  return *sp_;
}
