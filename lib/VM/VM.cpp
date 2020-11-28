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

#include "hustle/VM.hpp"
#include "city.h"
#include "hustle/Object.hpp"
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>

using namespace hustle;

Cell hustle::True;
Cell hustle::False;
cell_t hustle::Exit = 0;
Cell hustle::FalseWord;
TypedCell<Word> hustle::Mark;

namespace hustle {
void interpreter_break();
struct DebuggerInterface {
  volatile char code = 0;
  enum State { DBG_NONE, DBG_STEP, DBG_OVER } state;
  const void* call_stack = nullptr;
};

DebuggerInterface dbg_interface;
} // namespace hustle

static constexpr size_t MAX_CALL_FRAMES = 1024;

static TypedCell<Word> make_symbol_no_register(VM& vm, const char* n) {
  String* name = vm.allocate<String>(n, strlen(n));
  Array* definition = vm.allocate<Array>(1);
  Quotation* quote = vm.allocate<Quotation>();
  quote->definition = definition;
  quote->entry = nullptr;

  Word* word = vm.allocate<Word>();
  word->name = name;
  word->definition = quote;

  Wrapper* wrapper = vm.allocate<Wrapper>();
  wrapper->wrapped = Cell::from_raw(make_cell(word));
  // TODO: this is gross
  *(definition->begin()) = Cell::from_raw(make_cell(wrapper));
  return word;
}

static cell_t make_symbol(VM& vm, const char* name) {
  auto word = make_symbol_no_register(vm, name);
  vm.register_symbol(cast<String>(word->name), word);
  return word.raw();
}

thread_local VM* hustle::current_vm = nullptr;

VM::VM()
    : stack_(STACK_SIZE), call_stack_(MAX_CALL_FRAMES), heap_(this),
      lexer_(*this) {
  HSTL_ASSERT(current_vm == nullptr);
  current_vm = this;
  // memset(stack_, 0, STACK_SIZE);

  // Allocate our true and false values
  // TODO these maybe should be better
  // TODO should make these handles in case we gc
  // right now we just roll the dice that we almost certainly wont gc
  // TODO fixme
  True = Cell::from_raw(make_symbol(*this, "True"));

  False = Cell::from_raw(make_symbol(*this, "False"));
  {
    Array* a = allocate<Array>(1);
    (*a)[0] = False;
    auto* q = allocate<Quotation>();
    q->definition = a;
    // register_symbol();
    auto* w = allocate<Word>();
    w->definition = q;
    w->name = cast<Word>(False)->name;
    w->is_parse_word = true;
    symbol_table_["False"] = make_cell(w);
    FalseWord = w;
  }

  Exit = make_symbol(*this, "exit-bootstrap");
  Mark = make_symbol_no_register(*this, "<MARK>");
}

VM::~VM() {
  HSTL_ASSERT(current_vm != nullptr);
  current_vm = nullptr;
}

void VM::evaluate(Cell cell) {
  switch (cell.tag()) {
  case CELL_WRAPPER:
  case CELL_WORD: {
    return call(cell);
  }
  default:
    push(cell);
    break;
  }
}

void VM::call(Cell cell) {
  StackFrame frame;
  frame.word = False;
  frame.quote = TypedCell<Quotation>(nullptr);
  frame.offset = Cell::from_int(0);
  call_stack_.push(frame);

  switch (cell.tag()) {
  case CELL_WRAPPER: {
    // TODO should this be in call?
    Wrapper* wrapper = cast<Wrapper>(cell);
    HSTL_ASSERT(wrapper != nullptr);
    push(wrapper->wrapped);
    return;
  }
  case CELL_WORD:
    frame.word = cell;
    cell = cast<Word>(cell)->definition;

    // Fall through
  case CELL_QUOTE: {
    frame.quote = cell;
    call_stack_.push(frame);
    interpreter_loop();
    return;
  }
  default:
    HSTL_ASSERT(false);
  }
}

// TODO we need some way of signaling lookup failure
cell_t VM::lookup_symbol(const std::string& name) {
  auto it = symbol_table_.find(name);
  if (it == symbol_table_.end()) {
    // return make_cell<Word>(nullptr);
    std::cerr << "Symbol not found: " << name << "\n";
    throw std::runtime_error("symbol not found");
  } else {
    return it->second;
  }
}

void VM::register_symbol(String* string, Quotation* quote, bool parseword) {
  Word* word = allocate<Word>();
  word->name = string;
  word->definition = quote;
  word->is_parse_word = parseword;
  register_symbol(string, word);
}

void VM::register_symbol(String* string, Word* word) {
  std::string sys_name(string->data(), string->data() + string->length());
  symbol_table_.emplace(std::move(sys_name), make_cell(word));
}

void VM::step_hook() {
  bool should_break = false;
  switch (dbg_interface.state) {
  case DebuggerInterface::DBG_STEP:
    should_break = true;
    break;
  case DebuggerInterface::DBG_OVER:
    HSTL_ASSERT(dbg_interface.call_stack != nullptr);
    if (call_stack_.sp() == dbg_interface.call_stack) {
      should_break = true;
    }
    break;
  }

  if (should_break) {
    dbg_interface.state = DebuggerInterface::DBG_NONE;
    dbg_interface.code = 0;
    dbg_interface.call_stack = nullptr;
    interpreter_break();
    // Process the next debugger command
    switch (dbg_interface.code) {
    case 0:
      break;
    case 's':
      dbg_interface.state = DebuggerInterface::DBG_STEP;
      break;
    case 'o':
      dbg_interface.state = DebuggerInterface::DBG_OVER;
      dbg_interface.call_stack = call_stack_.sp();
    }

  } else {
    if (dbg_interface.code) {
      std::cerr << "DBG:: Warning, code is non-0, ignoring\n";
    }
  }
}

void VM::interpreter_loop() {
  while (call_stack_.begin() != call_stack_.end()) {
  loop_entry:
    auto frame = call_stack_[0];
    intptr_t offset = frame.offset.cast<intptr_t>();
    HSTL_ASSERT(offset >= 0);
    Quotation* quote = cast<Quotation>(frame.quote);
    if (quote == nullptr) {
      call_stack_.pop();
      return;
    }
    if (quote->entry != nullptr) {
      HSTL_ASSERT(offset == 0);
      quote->entry(this, nullptr);
      call_stack_.pop();
      continue;
    }

    Array* arr = cast<Quotation>(frame.quote)->definition;
    auto size = arr->count();
    HSTL_ASSERT(size <= INTPTR_MAX);
    for (; offset < size; ++offset) {
      step_hook();
      auto cell = (*arr)[offset];
      switch (cell.tag()) {
      case CELL_WORD: {
        // TODO need to do tail call recursion properly
        call_stack_.update_offset(offset + 1);

        auto word = cast<Word>(cell);
        StackFrame frame;
        frame.offset = Cell::from_int(0);
        frame.word = cell;
        frame.quote = cast<Word>(cell)->definition;
        call_stack_.push(frame);
        goto loop_entry;
      }
      case CELL_WRAPPER:
        push(cast<Wrapper>(cell)->wrapped);
        break;

      default:
        push(cell);
        break;
      }
    }
    call_stack_.pop();
  }
}

bool VM::is_parse_word(Word* w) const { return w->is_parse_word; }
