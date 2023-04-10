/*
 * Copyright (c) 2021, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "hustle/VM.hpp"
#include "city.h"
#include "hustle/Object.hpp"
#include "hustle/Parser/BootstrapLexer.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>

using namespace hustle;
using namespace std::literals;

namespace hustle {
void register_primitives(VM& vm);
struct DebuggerInterface {
  volatile char code = 0;
  enum State { DBG_NONE, DBG_STEP, DBG_OVER } state;
  const void* call_stack = nullptr;
};

DebuggerInterface dbg_interface;
} // namespace hustle

static constexpr size_t MAX_CALL_FRAMES = 1024;

static TypedCell<Word> make_symbol_no_register(VM& vm, const char* n) {
  auto definition = vm.allocate_handle<Array>(1);

  auto word = vm.allocate_handle<Word>();
  word->name = vm.allocate<String>(n, strlen(n));

  Quotation* quote = vm.allocate<Quotation>();
  quote->definition = definition;
  quote->entry = nullptr;

  word->definition = quote;

  Wrapper* wrapper = vm.allocate<Wrapper>();
  wrapper->wrapped = Cell::from_raw(make_cell(word));
  // TODO: this is gross
  *(definition->begin()) = Cell::from_raw(cell_helpers::make_cell(wrapper));

  // TODO: should this conversion be done implicitly?
  return TypedCell<Word>(word);
}

static TypedCell<Word> make_symbol(VM& vm, const char* name) {
  auto word = make_symbol_no_register(vm, name);
  vm.register_symbol(cast<String>(word->name), word);
  return word;
}

Word* VM::register_primitive(const char* name, CallType handler,
                             bool is_parse) {
  size_t name_len = strlen(name);
  auto word = allocate_handle<Word>();
  word->name = allocate<String>(name, name_len);
  word->definition = allocate<Quotation>(handler);
  word->is_parse_word = is_parse;
  symbol_table_.emplace(std::string(name), make_cell(word));
  return word;
}

thread_local VM* hustle::current_vm = nullptr;

VM::VM()
    : stack_(STACK_SIZE), call_stack_(MAX_CALL_FRAMES), lexer_(*this),
      heap_([this](Heap::MarkFunction fn) { mark_roots(fn); }) {
  HSTL_ASSERT(current_vm == nullptr);
  current_vm = this;
  // memset(stack_, 0, STACK_SIZE);

  // Allocate our global values
  globals.True = make_symbol(*this, "True");
  globals.False = make_symbol(*this, "False");
  globals.Exit = make_symbol(*this, "exit-bootstrap");
  globals.Mark = make_symbol_no_register(*this, "<MARK>");

  register_primitives(*this);
}

VM::~VM() {
  HSTL_ASSERT(current_vm != nullptr);
  current_vm = nullptr;
}

void VM::evaluate(Cell cell) {
  if (cell.is_a<Wrapper>() || cell.is_a<Word>()) {
    return call(cell);
  } else {
    push(cell);
  }
}

void VM::call(Cell cell) {
  current_vm = this;
  StackFrame frame;
  frame.word = nullptr;
  frame.quote = TypedCell<Quotation>(nullptr);
  frame.offset = Cell::from_int(0);
  call_stack_.push(frame);

  switch (cell.get_object()->tag()) {
  case OBJ_TAG_WRAPPER: {
    // TODO should this be in call?
    Wrapper* wrapper = cast<Wrapper>(cell);
    HSTL_ASSERT(wrapper != nullptr);
    push(wrapper->wrapped);
    return;
  }
  case OBJ_TAG_WORD:
    frame.word = cell;
    cell = cast<Word>(cell)->definition;

    // Fall through
  case OBJ_TAG_QUOTE: {
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
    std::cerr << "Symbol not found: '" << name << "'\n";
    throw std::runtime_error("symbol not found");
  } else {
    return it->second;
  }
}

void VM::register_symbol(String* string_raw, Quotation* quote_raw,
                         bool parseword) {
  Handle<String> string = make_handle<String>(string_raw);
  Handle<Quotation> quote = make_handle<Quotation>(quote_raw);
  Word* word = allocate<Word>();
  word->name = string;
  word->definition = quote;
  word->is_parse_word = parseword;
  register_symbol(string, word);
}

void VM::register_symbol(String* string, Word* word) {
  std::string sys_name(string->data(), string->data() + string->length());
  symbol_table_[std::move(sys_name)] = cell_helpers::make_cell(word);
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
  case DebuggerInterface::DBG_NONE:
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
    if (frame.quote.get_object() == nullptr) {
      call_stack_.pop();
      return;
    }
    Quotation* quote = cast<Quotation>(frame.quote);

    if (quote->entry != nullptr) {
      HSTL_ASSERT(offset == 0);
      quote->entry(this, nullptr);
      call_stack_.pop();
      continue;
    }

    Array* arr = cast<Quotation>(frame.quote)->definition;
    auto size = arr->count();
    HSTL_ASSERT(size <= INTPTR_MAX);
    for (; (uintptr_t)offset < size; ++offset) {
      step_hook();
      auto cell = (*arr)[offset];
      if (cell.is_a<Word>()) {
        // TODO: need to do tail call recursion properly
        call_stack_.update_offset(offset + 1);
        StackFrame frame;
        frame.offset = Cell::from_int(0);
        frame.word = cell;
        frame.quote = cast<Word>(cell)->definition;
        call_stack_.push(frame);
        goto loop_entry;
      } else if (cell.is_a<Wrapper>()) {
        push(cast<Wrapper>(cell)->wrapped);

      } else {
        push(cell);
      }
    }
    call_stack_.pop();
  }
}

bool VM::is_parse_word(Word* w) const { return w->is_parse_word; }

void VM::interpreter_break() {
  if (debug_listener_ != nullptr) {
    debug_listener_();
  }
}

void VM::mark_roots(Heap::MarkFunction fn) {

  fn((cell_t*)&globals.True);
  fn((cell_t*)&globals.False);
  fn((cell_t*)&globals.Exit);
  fn((cell_t*)&globals.Mark);

  for (Cell& slot : stack_) {
    fn((cell_t*)&slot);
  }
  for (auto& p : symbol_table_) {
    auto old = p.second;
    fn(&p.second);
    HSTL_ASSERT(p.second != old);
  }
  for (auto& frame : call_stack_) {
    fn((cell_t*)&frame.word);
    fn((cell_t*)&frame.quote);
  }
  handle_manager_.mark_handles(fn);
}

void VM::load_kernel() {
  auto kernel =
      std::make_unique<std::ifstream>(hustle::hustle_lib_dir() / "kernel.hsl");
  if (!kernel) {
    std::cerr << "Failed to load kernel\n";
    abort();
  }

  lexer_.add_stream(std::move(kernel));
  run();
  HSTL_ASSERT(stack_.depth() == 0);
}

VM* VM::get_current_vm() { return current_vm; }

void VM::run() {
  while (lexer_.current_stream()) {
    auto tok = lexer_.token();
    if (std::holds_alternative<intptr_t>(tok)) {
      push(Cell::from_int(std::get<intptr_t>(tok)));
    } else if (std::holds_alternative<std::string>(tok)) {
      std::string& str = std::get<std::string>(tok);
      auto result = lookup_symbol(str);
      evaluate(Cell::from_raw(result));
    } else {
      // Occurs if we hit an end of stream
      // Kinda clunky, but just continue
      continue;
    }
  }
}
