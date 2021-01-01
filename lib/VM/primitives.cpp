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

#include "hustle/Object.hpp"
#include "hustle/VM.hpp"
#include <hustle/Utility.hpp>
#include <utility>

#include <city.h>
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
using namespace hustle;

#include "primitives.def"

// TODO move to a header
namespace hustle {
void debug_break();
} // namespace hustle

void register_primitives(VM& vm) {
  for (auto x : primitives) {
    // String * name = vm.allocate<String>(x.first);
    size_t name_len = strlen(x.first);
    auto word = vm.allocate_handle<Word>();
    word->name = vm.allocate<String>(x.first, name_len);
    word->definition = vm.allocate<Quotation>(x.second);
    vm.symbol_table_.emplace(std::string(x.first), make_cell(word));
  }
}

static void prim_def(VM* vm, Quotation*) {
  auto definition = vm->pop();
  auto name = vm->pop();
  vm->register_symbol(cast<String>(name), cast<Quotation>(definition));
}

static void prim_defp(VM* vm, Quotation*) {
  auto definition = vm->pop();
  auto name = vm->pop();
  vm->register_symbol(cast<String>(name), cast<Quotation>(definition), true);
}

static void prim_arr_to_quote(VM* vm, Quotation*) {
  auto quote = vm->allocate<Quotation>();
  quote->definition = vm->pop();
  vm->push_obj(quote);
}

static void prim_lookup(VM* vm, Quotation*) {
  auto c = vm->pop();
  String* vm_str = cast<String>(c);
  // auto span = cast<String>(vm->pop())->to_span();
  std::string str(vm_str->data(), vm_str->length());
  vm->push(Cell::from_raw(vm->lookup_symbol(str)));
}

static void prim_hash(VM* vm, Quotation*) {
  auto a = vm->pop();
  switch (get_cell_type(a.raw())) {
  case CELL_STRING: {
    String* s = cast<String>(a);
    uint64_t hash = CityHash64(s->data(), s->length());
    vm->push(Cell::from_int(hash));
    return;
  }
  case CELL_INT: {
    vm->push(a);
    return;
  }
  default:
    HSTL_ASSERT(false);
  }
}

static void prim_make_record(VM* vm, Quotation*) {
  intptr_t slots = cast<intptr_t>(vm->pop());
  HSTL_ASSERT(slots > 0);
}

static void prim_empty_array(VM* vm, Quotation*) {
  intptr_t slots = cast<intptr_t>(vm->pop());
  HSTL_ASSERT(slots > 0);
  Array* array = vm->allocate<Array>((size_t)slots);
  vm->push_obj(array);
}

static void prim_length(VM* vm, Quotation*) {
  auto obj_cell = vm->pop();
  switch (obj_cell.tag()) {
  case CELL_INT:
    vm->push(Cell::from_int(1));
    break;
  case CELL_STRING: {
    vm->push(cast<String>(obj_cell)->length_raw);
    break;
  }
  case CELL_ARRAY: {
    // TODO: this is gross
    vm->push(Cell::from_int(cast<Array>(obj_cell)->count()));
    break;
  }
  default:
    vm->push(Cell::from_int(-1));
  }
}

static void prim_is_array(VM* vm, Quotation*) {
  auto cell = vm->pop();
  if (is_a<Array>(cell)) {
    vm->push(vm->globals.True);
  } else {
    vm->push(vm->globals.False);
  }
}

static void prim_make_sym(VM* vm, Quotation*) {
  auto name = vm->make_handle<String>(cast<String>(vm->pop()));
  auto definition = vm->allocate_handle<Array>(1);
  auto quote = vm->allocate_handle<Quotation>();
  quote->definition = definition;
  quote->entry = nullptr;

  auto word = vm->allocate_handle<Word>();
  word->name = name;
  word->definition = quote;

  Wrapper* wrapper = vm->allocate<Wrapper>();
  // TODO: gross
  wrapper->wrapped = Cell::from_raw(make_cell(word));
  *(definition->begin()) =
      Cell::from_raw(make_cell(wrapper)); // TODO: this is really gross
  vm->register_symbol(name, word);
}

static void prim_set_all(VM* vm, Quotation*) {
  auto value = vm->pop();
  Array* arr_ptr = cast<Array>(vm->pop());
  Array& arr = *arr_ptr;
  size_t arr_size = arr.count();
  for (size_t i = 0; i < arr_size; ++i) {
    arr[i] = value;
  }
}

static void prim_print(VM* vm, Quotation*) {
  String* str = cast<String>(vm->pop());
  std::string_view sv(str->data(), str->length());
  fmt::print("PRINT: '{}'\n", sv);
}

static void prim_mark_stack(VM* vm, Quotation*) { vm->push(vm->globals.Mark); }

static void prim_mark_to_array(VM* vm, Quotation*) {
  size_t count = -1;
  auto max_depth = vm->stack_.depth();
  for (size_t i = 0; i < max_depth; ++i) {
    if (vm->stack_[i] == vm->globals.Mark) {
      count = i;
      break;
    }
  }
  HSTL_ASSERT(count <= max_depth);
  auto arr_ptr = vm->allocate<Array>(count);
  Array& arr = *arr_ptr;
  for (size_t i = count; i > 0; --i) {
    arr[i - 1] = vm->stack_.pop();
  }
  Cell mark = vm->stack_.pop(); // Pop off the mark
  HSTL_ASSERT(mark == vm->globals.Mark);
  vm->stack_.push(TypedCell<Array>(arr_ptr));
}
static void prim_is_string(VM* vm, Quotation*) {
  auto arg = vm->pop();
  if (arg.is_a<String>()) {
    vm->push(vm->globals.True);
  } else {
    vm->push(vm->globals.False);
  }
}

/* #region  Control flow primitives */
static void prim_while(VM* vm, Quotation*) {
  // TODO while is broken
  // HSTL_ASSERT(false);
  auto body = vm->make_handle<Quotation>(cast<Quotation>(vm->pop()));
  auto condition = vm->make_handle<Quotation>(cast<Quotation>(vm->pop()));

  // HSTL_ASSERT(false);
  while (true) {
    vm->call(condition.cell());
    auto result = vm->pop();
    if (result == vm->globals.False)
      break;
    vm->call(body.cell());
  }
}

static void prim_ternary(VM* vm, Quotation*) {
  auto if_false = vm->pop();
  auto if_true = vm->pop();
  auto cond = vm->pop();
  vm->push((cond == vm->globals.False) ? if_false : if_true);
}

static void prim_exit(VM* vm, Quotation*) { exit(0); }

static void prim_call(VM* vm, Quotation*) {
  StackFrame frame;
  frame.word = nullptr;
  // Quotation* quote = vm->pop().cast<Quotation>();
  auto arg = vm->pop();
  if (arg.is_a<Word>()) {
    frame.word = arg;
    frame.quote = cast<Word>(arg)->definition;
  } else if (arg.is_a<Quotation>()) {
    frame.quote = arg;
  } else {
    prim_backtrace(vm, nullptr);
    HSTL_ASSERT(false);
  }
  HSTL_ASSERT(frame.quote.is_a<Quotation>());
  frame.offset = Cell::from_int(0);

  vm->call_stack_.pop();

  vm->call_stack_.push(frame);
  vm->call_stack_.push(frame);

  return;
}
/* #endregion */

/* #region  Stack Primitives */
static void prim_swap(VM* vm, Quotation*) {
  auto a = vm->pop();
  auto b = vm->pop();
  vm->push(a);
  vm->push(b);
}

static void prim_over(VM* vm, Quotation*) {
  auto a = vm->pop();
  auto b = vm->peek();
  vm->push(a);
  vm->push(b);
}

static void prim_dup(VM* vm, Quotation*) { vm->push(vm->peek()); }

static void prim_rot(VM* vm, Quotation*) {
  auto c = vm->pop();
  auto b = vm->pop();
  auto a = vm->pop();
  vm->push(c);
  vm->push(a);
  vm->push(b);
}

static void prim_pick(VM* vm, Quotation*) { vm->push(vm->stack_[2]); }

static void prim_dip(VM* vm, Quotation*) {
  auto quote = vm->pop();
  auto x = vm->make_handle(vm->pop());
  vm->call(quote);
  vm->push(x.cell());
}

static void prim_drop(VM* vm, Quotation*) { vm->pop(); }

/* #endregion */

/* #region  Arithmetic primitives */
static void prim_lt(VM* vm, Quotation*) {
  intptr_t b = cast<intptr_t>(vm->pop());
  intptr_t a = cast<intptr_t>(vm->pop());
  if (a < b) {
    vm->push(vm->globals.True);
  } else {
    vm->push(vm->globals.False);
  }
}

static void prim_gt(VM* vm, Quotation*) {
  intptr_t b = cast<intptr_t>(vm->pop());
  intptr_t a = cast<intptr_t>(vm->pop());
  if (a > b) {
    vm->push(vm->globals.True);
  } else {
    vm->push(vm->globals.False);
  }
}

static void prim_mult(VM* vm, Quotation*) {
  auto a = cast<intptr_t>(vm->pop());
  auto b = cast<intptr_t>(vm->pop());
  vm->push(Cell::from_int(a * b));
}

static void prim_and(VM* vm, Quotation*) {
  auto a = cast<intptr_t>(vm->pop());
  auto b = cast<intptr_t>(vm->pop());
  vm->push(Cell::from_int(a & b));
}

static void prim_or(VM* vm, Quotation*) {
  auto a = cast<intptr_t>(vm->pop());
  auto b = cast<intptr_t>(vm->pop());
  vm->push(Cell::from_int(a | b));
}

static void prim_mod(VM* vm, Quotation*) {
  intptr_t b = cast<intptr_t>(vm->pop());
  intptr_t a = cast<intptr_t>(vm->pop());
  vm->push(Cell::from_int(a % b));
}

// TODO figure out semantics, is this identity or value based?
static void prim_eq(VM* vm, Quotation*) {
  auto b = vm->pop();
  auto a = vm->pop();
  if (a == b) {
    vm->push(vm->globals.True);
    return;
  }
  // TODO we maybe want to keep these on the stack in case we gc
  // but at the moment we cant gc
  if (is_a<String>(a)) {
    if (is_a<String>(b)) {
      String* str_a = cast<String>(a);
      String* str_b = cast<String>(b);
      if (str_a->length() == str_b->length()) {
        if (memcmp(str_a->data(), str_b->data(), str_a->length()) == 0) {
          vm->push(vm->globals.True);
          return;
        }
      }
    }
  }
  vm->push(vm->globals.False);
}

static void prim_add(VM* vm, Quotation*) {
  auto b = vm->pop();
  auto a = vm->pop();
  // TODO allow other types
  intptr_t result = cast<intptr_t>(a) + cast<intptr_t>(b);
  vm->push(Cell::from_int(result));
}

static void prim_sub(VM* vm, Quotation*) {
  auto b = vm->pop();
  auto a = vm->pop();
  // TODO allow other types
  intptr_t result = cast<intptr_t>(a) - cast<intptr_t>(b);
  vm->push(Cell::from_int(result));
}

static void prim_bool(VM* vm, Quotation*) {
  auto a = vm->pop();
  if (a == vm->globals.False) {
    vm->push(vm->globals.False);
  } else {
    vm->push(vm->globals.True);
  }
}

/* #endregion */

/* #region  Raw slot access */
static void prim_set_raw_slot(VM* vm, Quotation*) {
  auto value = vm->pop();
  intptr_t idx = cast<intptr_t>(vm->pop()) + 1;
  Object* obj = cast<Object>(vm->pop());
  size_t sz = obj->size() / sizeof(Cell);
  HSTL_ASSERT(idx >= 0);
  HSTL_ASSERT((uintptr_t)idx < sz);

  ((Cell*)obj)[idx] = value;
}

static void prim_raw_slot(VM* vm, Quotation*) {
  cell_t idx = cast<intptr_t>(vm->pop()) + 1;
  Object* obj = cast<Object>(vm->pop());
  size_t sz = obj->size() / sizeof(Cell);
  HSTL_ASSERT(idx < sz);
  vm->push(((Cell*)obj)[idx]);
}

/* #endregion */

/* #region  Debug primitives */

static void prim_debug_break(VM* vm, Quotation*) { vm->interpreter_break(); }

static void prim_backtrace(VM* vm, Quotation*) {
  using namespace std::literals;
  fmt::print("======= Backtrace ======\n");
  const auto* end = vm->call_stack_.end();
  for (auto* frame = vm->call_stack_.begin(); frame < end; ++frame) {
    std::string_view word_name;
    if (frame->word.is_a<Word>()) {
      word_name = *cast<Word>(frame->word)->name;
    } else {
      word_name = "<Anonymous>"sv;
    }
    fmt::print("{}+{}\n", word_name, cast<intptr_t>(frame->offset));
  }

  // for ()
}

static void prim_dump_stack(VM* vm, Quotation*) { dump_stack(*vm, true); }

static void prim_assert(VM* vm, Quotation* q) {
  auto message = vm->pop();
  auto condition = vm->pop();
  if (condition == vm->globals.False) {
    if (message.is_a<String>()) {
      // TODO try to print string
    }

    std::cerr << "Assertion failure\n";
    std::cerr << "Stack\n";
    prim_dump_stack(vm, nullptr);
    prim_backtrace(vm, q);
    abort();
  }
}

/* #endregion */

/* #region  Parsing primitives */

static std::vector<Cell> tokenize(std::string& str, VM& vm) {
  std::istringstream s(str);
  std::vector<Cell> cells;
  while (!s.eof()) {
    // int c = peek
    // TODO check for string literal

    std::string tok;
    s >> tok;
    if (isdigit(tok[0])) {
      size_t size = 0;
      long long ll = std::stoll(tok, &size, 0);
      if (size == tok.length()) {
        cells.push_back(Cell::from_raw(ll));
        continue;
      }
    }
    cell_t word = vm.lookup_symbol(tok);
    if (is_a<Word>(word) && get_cell_pointer(word) == nullptr) {
      std::cerr << "Unkown word " << tok << "\n";
      HSTL_ASSERT(false); // TODO need better error signaling
      continue;
    }
    cells.push_back(Cell::from_raw(word));
  }
  return cells;
}

static void prim_read_line(VM* vm, Quotation*) {
  std::cout << "(primitive-readline)> ";
  std::string line;
  std::getline(std::cin, line);
  String* vm_str = vm->allocate<String>(line.c_str(), line.length());
  vm->push_obj(vm_str);
}

static void prim_parse_str(VM* vm, Quotation*) {
  auto c = vm->pop();
  String* vm_str = cast<String>(c);
  std::string str(vm_str->data(), vm_str->length());
  auto tokens = tokenize(str, *vm);

  Array* arr = vm->allocate<Array>(tokens.size());
  // cell_t *arr_data = arr->data();
  std::copy(tokens.begin(), tokens.end(), arr->data());
  vm->push_obj(arr);
}

static void prim_is_parse_word(VM* vm, Quotation*) {

  auto val = vm->pop();
  HSTL_ASSERT(val.is_a<Word>());

  if (vm->is_parse_word(val.cast<Word>())) {
    vm->push(vm->globals.True);
  } else {
    vm->push(vm->globals.False);
  }
}

static void prim_lex_token(VM* vm, Quotation*) { vm->lexer_.lex_token(); }

static void prim_read_until(VM* vm, Quotation*) {
  auto terminator = cast<String>(vm->pop());

  HSTL_ASSERT(terminator->length() == 1);
  std::string s = vm->lexer_.read_until(*terminator->data());
  auto vm_str = vm->allocate<String>(s.length() + 1);
  std::copy(s.begin(), s.end(), vm_str->data());
  vm_str->length_raw = Cell::from_int(s.length());
  vm->push(vm_str);
}

static void prim_include(VM* vm, Quotation*) {
  String* str = cast<String>(vm->pop());
  std::string filename(str->data(), str->length());
  auto fstream = std::make_unique<std::ifstream>(filename);
  HSTL_ASSERT(*fstream);

  vm->lexer_.add_stream(std::move(fstream));
}

/* #endregion */
