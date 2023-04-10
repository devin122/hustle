/*
 * Copyright (c) 2020, Devin Nakamura
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
#include "hustle/VM/Cell.hpp"
#include <map>
#include <memory>
#include <string>
#include <type_traits>

namespace hustle {

struct Quotation;
struct String;
struct Word;

class Lexer;

struct VM {
  using CallType = void (*)(VM*, Quotation*);
  static constexpr size_t STACK_SIZE = 4096;
  // public:
  VM();
  ~VM();
  VM(const VM&) = delete;
  VM& operator=(const VM&) = delete;

  // TODO: this is pretty gross
  struct {
    TypedCell<Word> True;
    TypedCell<Word> False;
    TypedCell<Word> Exit;
    TypedCell<Word> Mark;
  } globals;

  void evaluate(Cell c) HUSTLE_MAY_ALLOCATE;

  void interpreter_loop();
  void step_hook();

  void push(Cell cell) { stack_.push(cell); }
  Cell pop() { return stack_.pop(); }
  Cell peek() const { return stack_.peek(); }

  // CallStackEntry enter(Word)

  void call(Cell c) HUSTLE_MAY_ALLOCATE;

  // hack because im lazy
  template <typename T>
  void push_obj(T* o) {
    push(Cell(o));
  }

  Word* register_primitive(const char* name, CallType handler,
                           bool is_parse = false) HUSTLE_MAY_ALLOCATE;
  void register_symbol(String* str, Quotation* quote,
                       bool parseword = false) HUSTLE_MAY_ALLOCATE;
  void register_symbol(String* string, Word* word);

  Cell lookup_symbol(const std::string& name);
  // private:
  // TODO do these really need to be functions?

  // TODO: this should probably be called automatically
  void load_kernel();

  void run();

  Stack stack_;
  CallStack call_stack_;

  std::map<std::string, cell_t> symbol_table_;

  Lexer lexer_;
  Heap heap_;

  void mark_roots(Heap::MarkFunction fn);

  template <typename T, typename... Args>
  // std::enable_if_t<std::is_base_of_v<Object, T>, T*>
  T* allocate(Args... args) HUSTLE_MAY_ALLOCATE {
    size_t allocation_size = hustle::object_allocation_size(
        (T*)nullptr, std::forward<Args>(args)...);
    void* memory = heap_.allocate(allocation_size);
    return new (memory) T(std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  Handle<T> allocate_handle(Args... args) HUSTLE_MAY_ALLOCATE {
    return make_handle(allocate<T>(std::forward<Args>(args)...));
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

  template <typename T>
  Handle<T> make_handle(T* ptr) {
    return handle_manager_.make_handle(ptr);
  }

  template <typename T>
  Handle<T> make_handle(TypedCell<T>& cell) {
    return handle_manager_.make_handle((T*)cell);
  }

  HandleBase make_handle(Cell c) { return handle_manager_.make_handle(c); }
  static VM* get_current_vm();

private:
  DebugListener debug_listener_ = nullptr;
  HandleManager handle_manager_;
};

// TODO: currently only allowed 1 per thread. should allow context switching
// between vms
extern thread_local VM* current_vm;

} // namespace hustle
#endif
