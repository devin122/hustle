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

#include <catch2/catch.hpp>
// #include <hustle/Object.hpp>
// #include <hustle/cell.hpp>
#include <hustle/VM.hpp>

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

  auto def_func = vm.lookup_symbol("def");
  REQUIRE(def_func != 0);

  vm.push(allocate_string(vm, "add3"sv));

  auto mark_func = vm.lookup_symbol("mark-stack");
  REQUIRE(mark_func != 0);
  vm.call(Cell::from_raw(mark_func));

  auto add_func = vm.lookup_symbol("+");
  REQUIRE(add_func != 0);
  vm.push(Cell::from_int(3));
  vm.push(Cell::from_raw(add_func));

  auto mark2arr_func = vm.lookup_symbol("mark>array");
  REQUIRE(mark2arr_func != 0);
  vm.call(Cell::from_raw(mark2arr_func));

  auto arr2quote_func = vm.lookup_symbol("array>quote");
  REQUIRE(arr2quote_func != 0);
  vm.call(Cell::from_raw(arr2quote_func));

  vm.call(Cell::from_raw(def_func));

  REQUIRE(vm.stack_.begin() == vm.stack_.end());
  auto add3_func = vm.lookup_symbol("add3");
  REQUIRE(add3_func != 0);

  vm.push(Cell::from_int(5));
  vm.call(Cell::from_raw(add3_func));
  REQUIRE(vm.pop() == Cell::from_int(8));
}
