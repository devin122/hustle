/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#ifndef HUSTLE_STACK_HPP
#define HUSTLE_STACK_HPP

#include "hustle/Support/Error.hpp"
#include "hustle/VM/Cell.hpp"

namespace hustle {

/// Repesent stacks in the hustle language
/// The data stack uses this class directly, while the call stack uses this as a
/// base class
class Stack {
public:
  /// Create stack with a given size
  /// \param sz Size of the stack in slots
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
  }

  constexpr size_t depth() const { return top_ - sp_; }

  Cell peek() const;
  Cell* begin() { return sp_; }
  const Cell* cbegin() const { return sp_; }
  Cell* end() { return top_; }
  const Cell* cend() const { return top_; }

  const Cell* sp() const { return sp_; }
  void clear();

protected:
  Cell* base_;
  Cell* sp_;
  Cell* top_;
};

// TODO: this should maybe be a record type?
// TODO: make sure this is packed
/***
 * Frame in hustle the call stack
 */
struct StackFrame {
  /// calling/called word, or null if anonymous quote
  TypedCell<Word> word;

  /// calling/called quote, or null for an interpreter entry frame
  TypedCell<Quotation> quote;

  /// Offset in the quote definition
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

/***
 * Track the call stack of hustle
 *
 * \todo This should probaly be moved to a hustle-native datastructure
 */
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

  StackFrame* begin() {
    HSTL_ASSERT((top_ - sp_) % 3 == 0);
    return (StackFrame*)sp_;
  }
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
  HSTL_ASSERT(sp_ <= top_);

  HSTL_ASSERT((uintptr_t)(top_ - sp_) >=
              (idx * (sizeof(StackFrame) / sizeof(Cell))));
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
