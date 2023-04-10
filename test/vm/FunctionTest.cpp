/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "hustle/VM.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace hustle;
using namespace std::literals;

// Hack to work with broken string allocation code
static String* allocate_string(VM& vm, std::string_view sv) {
  const char* data = sv.data();
  size_t sz = sv.size();
  return vm.allocate<String>(data, sz);
}

TEST_CASE("Can define function", "[function]") {
  VM vm;

  // TODO: some of these should be handles since we could technically allocate
  auto def_func = vm.lookup_symbol("def");
  REQUIRE(def_func != Cell());

  vm.push(allocate_string(vm, "add3"sv));

  auto mark_func = vm.lookup_symbol("mark-stack");
  REQUIRE(mark_func != Cell());
  vm.call(mark_func);

  auto add_func = vm.lookup_symbol("+");
  REQUIRE(add_func != Cell());
  vm.push(Cell::from_int(3));
  vm.push(add_func);

  auto mark2arr_func = vm.lookup_symbol("mark>array");
  REQUIRE(mark2arr_func != Cell());
  vm.call(mark2arr_func);

  auto arr2quote_func = vm.lookup_symbol("array>quote");
  REQUIRE(arr2quote_func != Cell());
  vm.call(arr2quote_func);

  vm.call(def_func);

  REQUIRE(vm.stack_.begin() == vm.stack_.end());
  auto add3_func = vm.lookup_symbol("add3");
  REQUIRE(add3_func != Cell());

  vm.push(Cell::from_int(5));
  vm.call(add3_func);
  REQUIRE(vm.pop() == Cell::from_int(8));
}
