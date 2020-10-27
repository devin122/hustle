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
#include <hustle/Object.hpp>
#include <hustle/cell.hpp>

#include <iostream>
#include <random>

using namespace hustle;

// Helper because built in random generator from catch only supports ints

#define CELL_GENERATE                                                          \
  Catch::Generators::generate(                                                 \
      INTERNAL_CATCH_STRINGIZE(INTERNAL_CATCH_UNIQUE_NAME(generator)),         \
      CATCH_INTERNAL_LINEINFO, [] {                                            \
        using namespace Catch::Generators;                                     \
        return makeGenerators(__VA_ARGS__);                                    \
      })

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

  Cell s1 = Cell::from_raw(make_cell<String>(nullptr));
  Cell a1 = Cell::from_raw(make_cell<Array>(nullptr));

  CHECK(a1 != s1);
  CHECK_FALSE(a1 == s1);
  CHECK_FALSE(a1 == Cell::from_int(0));

  CHECK(ci1 == Cell::from_int(1));
}
