/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <catch2/catch.hpp>

#include <hustle/VM.hpp>
#include <string>

using namespace hustle;
using namespace std::literals;

// Helper function for calling hustle functions
static void call(VM& vm, const std::string& name) {
  auto sym = vm.lookup_symbol(name);
  REQUIRE(sym != 0);
  vm.call(Cell::from_raw(sym));
}

TEST_CASE("Primitives are registered", "[Primitive]") {
  VM vm;

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

TEST_CASE("Stack primitives", "[Primitive]") {
  VM vm;
  REQUIRE(vm.stack_.begin() == vm.stack_.end());

  SECTION("swap") {
    vm.push(Cell::from_int(2));
    vm.push(Cell::from_int(32));
    call(vm, "swap");
    REQUIRE(vm.pop() == Cell::from_int(2));
    REQUIRE(vm.pop() == Cell::from_int(32));
  }

  SECTION("dup") {
    vm.push(Cell::from_int(123));
    call(vm, "dup");
    REQUIRE(vm.pop() == Cell::from_int(123));
    REQUIRE(vm.pop() == Cell::from_int(123));
  }

  SECTION("over") {
    vm.push(Cell::from_int(16));
    vm.push(Cell::from_int(-1));
    call(vm, "over");

    REQUIRE(vm.pop() == Cell::from_int(16));
    REQUIRE(vm.pop() == Cell::from_int(-1));
    REQUIRE(vm.pop() == Cell::from_int(16));
  }

  SECTION("rot") {
    vm.push(Cell::from_int(-42));
    vm.push(Cell::from_int(19));
    vm.push(Cell::from_int(1));

    call(vm, "rot");

    REQUIRE(vm.pop() == Cell::from_int(19));
    REQUIRE(vm.pop() == Cell::from_int(-42));
    REQUIRE(vm.pop() == Cell::from_int(1));
  }

  REQUIRE(vm.stack_.begin() == vm.stack_.end());
}
