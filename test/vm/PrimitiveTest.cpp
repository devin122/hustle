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

#include <hustle/VM.hpp>
#include <string>

using namespace hustle;
using namespace std::literals;
void register_primitives(VM& vm); // TODO: this is a hack

// Helper function for calling hustle functions
static void call(VM& vm, const std::string& name) {
  auto sym = vm.lookup_symbol(name);
  REQUIRE(sym != 0);
  vm.call(Cell::from_raw(sym));
}

TEST_CASE("Primitives are registered", "[Primitive]") {
  VM vm;
  register_primitives(vm);

  auto prim_names = {"+"s, "-"s, "?"s, "def"s};

  for (const auto& name : prim_names) {
    DYNAMIC_SECTION("Primitive " << name) {
      auto prim = vm.lookup_symbol(name);
      REQUIRE(prim != 0);
      REQUIRE(is_a<Word>(prim));
      auto word = cast<Word>(prim);
      REQUIRE(word != nullptr);
      // TODO exit is not implemented?
      if (name != "exit"s) {
        REQUIRE(word->definition != nullptr);
        REQUIRE(word->definition->entry != nullptr);
      }
      // TODO: need to implement string comparison
      // REQUIRE(word->name == name);
    }
  }
}

TEST_CASE("Arithmetic primitives", "[Primitive]") {
  VM vm;
  register_primitives(vm);

  REQUIRE(vm.stack_.begin() == vm.stack_.end());
  SECTION("prim +") {
    vm.push(Cell::from_int(1));
    vm.push(Cell::from_int(2));
    call(vm, "+");
    REQUIRE(vm.pop() == Cell::from_int(3));
  }

  SECTION("prim -") {
    vm.push(Cell::from_int(2));
    vm.push(Cell::from_int(3));
    call(vm, "-");
    REQUIRE(vm.pop() == Cell::from_int(-1));
  }

  SECTION("prim *") {
    vm.push(Cell::from_int(-4));
    vm.push(Cell::from_int(32));
    call(vm, "*");
    REQUIRE(vm.pop() == Cell::from_int(-128));
  }
  REQUIRE(vm.stack_.begin() == vm.stack_.end());
}
