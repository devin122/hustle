/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <fmt/core.h>
#include <hustle/Object.hpp>
#include <hustle/Support/IndentingStream.hpp>
#include <hustle/VM.hpp>
#include <hustle/VM/cell.hpp>
#include <iostream>
#include <sstream>
#include <stdint.h>

using namespace hustle;
using namespace std::literals;

static constexpr size_t MAX_RECURSION = 8;

static std::string terse(intptr_t i) { return fmt::format("{}", i); }
template <typename T>
static std::string terse(T* ptr) {
  return fmt::format("{}<{}>", get_object_name(T::TAG_VALUE), (void*)ptr);
}
static std::string terse(String* s) {
  return fmt::format("\"{}\"", std::string_view(s->data(), s->length()));
}
static std::string terse(Word* w) {
  return fmt::format("{}",
                     std::string_view(w->name->data(), w->name->length()));
}
/*
static std::string terse(Array* a){
    const size_t elements = a->count();
    if(elements == 0) {
        return "[ ]"s;
    }
    std::sstream  strm;
    strm << "[ ";
    for(size_t sz =)
}*/
static std::string terse_cell(cell_t cell) {
  return dispatch_cell(cell, [](auto x) { return terse(x); });
}

static void print_cell(IndentingStream& out, Cell cell, std::string pfx = ""s,
                       bool recurse = false);
static const std::string describe_cell(intptr_t val, bool) {
  return fmt::format("{}", val);
}
static const std::string describe_cell(String* s, bool) {
  if (s == nullptr)
    return "nil"s;
  return fmt::format("@{} \"{}\"", (void*)s,
                     std::string_view(s->data(), s->length()));
}
static const std::string describe_cell(Object* p, bool) {
  return fmt::format("@{}", (void*)p);
}
static const std::string describe_cell(Array* a, bool) {
  if (a == nullptr)
    return "nil"s;
  return fmt::format("@{} elements={}", (void*)a,
                     (a->size() - sizeof(Array)) / sizeof(cell_t));
}

static const std::string describe_cell(Word* w, bool recurse) {
  if (recurse) {
    return describe_cell((Object*)w, recurse);
  } else {
    String* s = w->name;
    std::string_view sv =
        (s == nullptr) ? ""sv : std::string_view(s->data(), s->length());
    return fmt::format("@{} - {}", (void*)w, sv);
  }
}

static void visit_children(IndentingStream& out, Object* o) {}
static void visit_children(IndentingStream& out, intptr_t) {}
static void visit_children(IndentingStream& out, Array* a) {
  if (a == nullptr)
    return;
  // unsigned i = 0;
  size_t arr_size = a->count();
  for (size_t i = 0; i < arr_size; ++i) {
    print_cell(out, (*a)[i], fmt::format("[{}] - ", i), true);
  }
}
static void visit_children(IndentingStream& out, Quotation* quote) {
  print_cell(out, quote->definition, "definition: ", true);
  out.writeln("entry: {}", (void*)quote->entry);
}
static void visit_children(IndentingStream& out, Word* word) {
  //  TypedCell<String> name;
  // TypedCell<Quotation> definition;
  if (word == nullptr)
    return;
  print_cell(out, word->name, "Name: "s, true);
  print_cell(out, word->definition, "Definition: "s, true);
}

static void visit_children(IndentingStream& out, Wrapper* wrapper) {
  if (wrapper == nullptr)
    return;
  print_cell(out, wrapper->wrapped, "Wrapped: "s, true);
}

static void print_cell(IndentingStream& out, Cell cell, std::string pfx,
                       bool recurse) {
  out.writeln("{}({}) - {}", pfx, get_type_name(cell),
              dispatch_cell(cell.raw(),
                            [=](auto x) { return describe_cell(x, recurse); }));
  if (recurse) {
    if (out.depth() > MAX_RECURSION) {
      out.writeln("...");
      return;
    }
    out.indent();
    dispatch_cell(cell.raw(), [&](auto x) { visit_children(out, x); });
    out.outdent();
  }
}

void hustle::dump_stack(const VM& vm, bool recurse) {

  IndentingStream out(std::cout);
  // out.writeln("stack_base = {}, stack = {}", (void*)vm.stack_base_,
  // (void*)vm.stack_);

  if (vm.stack_.cbegin() == vm.stack_.cend()) {
    out.writeln("** stack empty **");
    return;
  }
  const Cell* sp = vm.stack_.cbegin();
  for (const Cell* ptr = vm.stack_.cend() - 1; ptr >= sp; --ptr) {
    ptrdiff_t idx = ptr - sp;
    // printf("<SP-%d> ", (int)idx - 1);
    print_cell(out, *ptr, fmt::format("<SP-{}> ", idx - 1), recurse);
  }
}

void hustle::print_stack(const VM& vm, unsigned max_depth) {
  ptrdiff_t depth = vm.stack_.cend() - vm.stack_.cbegin();

  HSTL_ASSERT(depth >= 0);
  if (max_depth < depth) {

    depth = max_depth;
  }
  for (int i = 0; i < depth; ++i) {
    std::cout << terse_cell(vm.stack_[i].raw()) << "\n";
  }
}