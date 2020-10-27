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
#ifndef HUSTLE_STACK_HPP
#define HUSTLE_STACK_HPP

#include <hustle/Support/Error.hpp>
#include <hustle/cell.hpp>

namespace hustle {

class Stack {
public:
  Stack(size_t sz);
  Stack(const Stack&) = delete;
  Stack& operator=(const Stack&) = delete;
  ~Stack();
  void push(Cell cell);
  void push(cell_t raw);

  Cell pop();
  template <typename T>
  T* pop_obj() {
    return pop().cast<T>();
  }

  Cell& operator[](uintptr_t idx) {
    HSTL_ASSERT(top_ >= sp_);
    if (idx >= depth()) {
      throw Exception("underflow");
    }
    HSTL_ASSERT(sp_ >= base_);
    HSTL_ASSERT(sp_ < top_);
    return sp_[idx];
  }
  const Cell& operator[](uintptr_t idx) const {
    HSTL_ASSERT(top_ >= sp_);
    if (idx >= depth()) {
      throw Exception("underflow");
    }
    HSTL_ASSERT(sp_ >= base_);
    HSTL_ASSERT(sp_ < top_);
    return sp_[idx];
    ;
  }

  constexpr size_t depth() const { return top_ - sp_; }

  Cell peek();
  Cell* begin() { return sp_; }
  const Cell* cbegin() const { return sp_; }
  Cell* end() { return top_; }
  const Cell* cend() const { return top_; }

  const Cell* sp() const { return sp_; }

protected:
  Cell* base_;
  Cell* sp_;
  Cell* top_;
};

// TODO: this should maybe be a record type?
// TODO: make sure this is packed
struct StackFrame {
  Cell word;
  Cell quote;
  Cell offset;
};

class CallStack;
class CallStackEntry {
public:
  CallStackEntry(CallStack& call_stack);
  ~CallStackEntry();

private:
  CallStack& my_stack_;
  // TODO this really just a debugging thing
  Cell* entry_sp_;
};

class CallStack : private Stack {

public:
  CallStack(size_t frame_ct)
      : Stack(frame_ct * (sizeof(StackFrame) / sizeof(Cell))) {}
  void push(const StackFrame& frame);

  // This is a hack to allow us to unwind the call stack after a C++ exception
  struct State;

  State get_state();
  void restore_state(State);
  void update_offset(size_t offset);
  StackFrame pop();

  StackFrame* begin() { return (StackFrame*)sp_; }
  StackFrame* end() { return (StackFrame*)top_; }

  using Stack::sp;
  // TODO there seems like safety issue with this call
  StackFrame& operator[](uintptr_t idx);
  StackFrame peek() { return (*this)[0]; }
};

struct CallStack::State {
  Cell* sp = nullptr;
};

inline CallStack::State CallStack::get_state() {
  State s;
  s.sp = sp_;
  return s;
}

inline StackFrame& CallStack::operator[](uintptr_t idx) {
  HSTL_ASSERT((sp_ - top_) >= (idx * (sizeof(StackFrame) / sizeof(Cell))));
  return *(((StackFrame*)sp_) + idx);
}
// TODO this is really inefficient
inline void CallStack::update_offset(size_t offset) {
  StackFrame frame = pop();
  frame.offset = Cell::from_int(offset);
  push(frame);
}
inline void CallStack::restore_state(State s) {
  HSTL_ASSERT(s.sp != nullptr);
  HSTL_ASSERT(s.sp > sp_);
  memset(sp_, 0, (s.sp - sp_) * sizeof(*sp_));
  sp_ = s.sp;
}

inline void CallStack::push(const StackFrame& f) {
  Stack::push(f.offset);
  Stack::push(f.quote);
  Stack::push(f.word);
}

inline StackFrame CallStack::pop() {
  StackFrame f;
  f.word = Stack::pop();
  f.quote = Stack::pop();
  f.offset = Stack::pop();
  return f;
}

inline CallStackEntry::CallStackEntry(CallStack& stack)
    : my_stack_(stack), entry_sp_(stack.get_state().sp) {}

inline CallStackEntry::~CallStackEntry() {
  HSTL_ASSERT(my_stack_.get_state().sp == entry_sp_);
  my_stack_.pop();
}
} // namespace hustle

#endif
