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
