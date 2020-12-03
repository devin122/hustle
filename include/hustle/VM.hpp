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

#ifndef HUSTLE_VM_HPP
#define HUSTLE_VM_HPP

#include "hustle/GC.hpp"
#include "hustle/Object.hpp"
#include "hustle/Parser/Lexer.hpp"
#include "hustle/Stack.hpp"
#include "hustle/Support/Error.hpp"
#include "hustle/cell.hpp"
#include <map>
#include <memory>
#include <string>
#include <type_traits>

namespace hustle {

// TODO these should be somewhere else
extern Cell True;
extern Cell False;
extern Cell FalseWord;
extern cell_t Exit;

struct Quotation;
struct String;
struct Word;
extern TypedCell<Word> Mark;

class Lexer;

struct VM {
  using CallType = void (*)(VM*, Quotation*);
  static constexpr size_t STACK_SIZE = 4096;
  // public:
  VM();
  ~VM();
  VM(const VM&) = delete;
  VM& operator=(const VM&) = delete;

  void evaluate(Cell c);

  void interpreter_loop();
  void step_hook();

  void push(Cell cell) { stack_.push(cell); }
  Cell pop() { return stack_.pop(); }
  Cell peek() const { return stack_.peek(); }

  // CallStackEntry enter(Word)

  void call(Cell c);

  // hack because im lazy
  template <typename T>
  void push_obj(T* o) {
    push(Cell(o));
  }

  Word* register_primitive(const char* name, CallType handler);
  void register_symbol(String* str, Quotation* quote, bool parseword = false);
  void register_symbol(String* string, Word* word);

  cell_t lookup_symbol(const std::string& name);
  // private:
  // TODO do these really need to be functions?

  Stack stack_;
  CallStack call_stack_;

  std::map<std::string, cell_t> symbol_table_;

  Lexer lexer_;
  Heap heap_;

  template <typename T>
  void mark_roots(T fn);

  template <typename T, typename... Args>
  // std::enable_if_t<std::is_base_of_v<Object, T>, T*>
  T* allocate(Args... args) {
    size_t allocation_size =
        hustle::object_allocation_size<T>(std::forward<Args>(args)...);
    void* memory = heap_.allocate(allocation_size);
    return new (memory) T(std::forward<Args>(args)...);
  }

  // TODO: this doesnt need to live in the vm
  bool is_parse_word(Word*) const;

  void dump_back_trace();

  typedef void (*DebugListener)();

  DebugListener set_debug_listener(DebugListener new_listener) {
    auto old_listener = debug_listener_;
    debug_listener_ = new_listener;
    return old_listener;
  }
  void interpreter_break();

private:
  DebugListener debug_listener_ = nullptr;
};

// TODO: currently only allowed 1 per thread. should allow context switching
// between vms
extern thread_local VM* current_vm;

template <typename T>
inline void VM::mark_roots(T fn) {
  HSTL_ASSERT(false); // TODO reimplement
#if 0
    for(cell_t* ptr = stack_base_; ptr < stack_; ++ptr){
       /* cell_t cell = *ptr;
        if(is_a<Object>(cell)){
            fn(cast<Object>(cell));
        }*/
        fn(ptr);
    }
    for(auto p : symbol_table_){
        //cell_t cell = p.second;
        fn(&p.second);
        /*if(is_a<Object>(cell)){
            fn(cast<Object>(cell));
        }*/
    }
#endif
}

} // namespace hustle
#endif
