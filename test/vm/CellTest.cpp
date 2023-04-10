/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <catch2/catch.hpp>
#include <hustle/Object.hpp>
#include <hustle/VM/Cell.hpp>

#include <iostream>
#include <random>

using namespace hustle;

TEST_CASE("Cell is initialized with default value", "[Cell]") {
  Cell c;
  REQUIRE(c.raw() == 0);
}

TEST_CASE("Cell::from_int", "[Cell]") {
  // Note the awkward value<intptr_t>'s are to ensure that the catch generator
  // doesnt do weird truncation
  intptr_t i =
      GENERATE(value<intptr_t>(0), 1, -1, value<intptr_t>(CELL_INT_MAX + 0),
               value<intptr_t>(CELL_INT_MIN + 0));
  auto cell = Cell::from_int(i);
  REQUIRE(cell.is_a<intptr_t>());
  REQUIRE(is_a<intptr_t>(cell.raw()));
  CHECK(cell.cast<intptr_t>() == i);
  CHECK(cast<intptr_t>(cell.raw()) == i);
}

TEST_CASE("Cell wrap arround", "[Cell]") {
  auto cell = Cell::from_int(CELL_INT_MAX + 1);
  REQUIRE(cell.is_a<intptr_t>());
  REQUIRE(is_a<intptr_t>(cell.raw()));
  CHECK(cell.cast<intptr_t>() == CELL_INT_MIN);

  cell = Cell::from_int(CELL_INT_MIN - 1);
  REQUIRE(cell.is_a<intptr_t>());
  REQUIRE(is_a<intptr_t>(cell.raw()));
  CHECK(cell.cast<intptr_t>() == CELL_INT_MAX);
}

TEST_CASE("Cell::operator==", "[Cell]") {
  Cell ci1 = Cell::from_int(1);
  Cell ci1_2 = Cell::from_int(1);
  Cell ci2 = Cell::from_int(2);

  CHECK(ci1 == ci1_2);
  CHECK(ci1_2 == ci1);
  CHECK(ci1 == ci1);
  CHECK_FALSE(ci1 == ci2);

  CHECK_FALSE(ci1 != ci1_2);
  CHECK_FALSE(ci1 != ci1);
  CHECK(ci1 != ci2);
  CHECK(ci1 == Cell::from_int(1));
}
