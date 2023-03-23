/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "hustle/Object.hpp"
#include "hustle/VM.hpp"

#include <catch2/catch.hpp>

using namespace hustle;
TEST_CASE("Stack throws on underflow", "[Stack]") {
  Stack stk(32);
  CHECK_THROWS(stk.pop());
  CHECK_THROWS(stk.peek());

  CHECK_NOTHROW(stk.push(Cell::from_int(42)));
  CHECK_NOTHROW(stk.peek());
  CHECK_NOTHROW(stk.pop());

  CHECK_THROWS(stk.peek());
  CHECK_THROWS(stk.pop());
}

TEST_CASE("Stack throws on overflow", "[Stack]") {
  Stack stk(3);

  Cell c = Cell::from_raw(0);
  CHECK_NOTHROW(stk.push(c));
  CHECK_NOTHROW(stk.push(c));
  CHECK_NOTHROW(stk.push(c));
  CHECK_THROWS(stk.push(c));
  stk.pop();
  CHECK_NOTHROW(stk.push(c));
}

TEST_CASE("Stack::operator[]", "[Stack]") {
  Stack stk(4);
  Cell c1 = Cell::from_int(1);
  Cell c2 = Cell::from_int(2);
  Cell c3 = Cell::from_raw(make_cell<String>(nullptr));
  Cell c4 = Cell::from_raw(make_cell<Word>(nullptr));

  CHECK_THROWS(stk[0]);
  CHECK_THROWS(stk[1]);
  CHECK_THROWS(stk[-1]);

  stk.push(c4);
  CHECK(stk[0] == c4);
  CHECK_THROWS(stk[1]);
  CHECK_THROWS(stk[-1]);

  stk.push(c3);
  stk.push(c2);
  stk.push(c1);

  CHECK(stk[0] == c1);
  CHECK(stk[1] == c2);
  CHECK(stk[2] == c3);
  CHECK(stk[3] == c4);

  stk[1] = c1;
  CHECK(stk[1] == c1);
  stk.pop();
  CHECK(stk.peek() == c1);
}

TEST_CASE("const Stack::operator[]", "[Stack]") {
  Stack mutable_stack(4);
  Cell c1 = Cell::from_int(1);
  Cell c2 = Cell::from_int(2);
  Cell c3 = Cell::from_raw(make_cell<String>(nullptr));
  Cell c4 = Cell::from_raw(make_cell<Word>(nullptr));

  {
    const Stack& stk = mutable_stack;
    CHECK_THROWS(stk[0]);
    CHECK_THROWS(stk[1]);
    CHECK_THROWS(stk[-1]);
  }
  mutable_stack.push(c4);

  {
    const Stack& stk = mutable_stack;
    CHECK(stk[0] == c4);
    CHECK_THROWS(stk[1]);
    CHECK_THROWS(stk[-1]);
  }
  mutable_stack.push(c3);
  mutable_stack.push(c2);
  mutable_stack.push(c1);

  {
    const Stack& stk = mutable_stack;
    CHECK(stk[0] == c1);
    CHECK(stk[1] == c2);
    CHECK(stk[2] == c3);
    CHECK(stk[3] == c4);
  }
}

TEST_CASE("Stack begin() and end()", "[Stack]") {
  Stack stack(10);
  CHECK(stack.begin() == stack.end());
  CHECK(stack.begin() == stack.cbegin());
  CHECK(stack.end() == stack.cend());

  stack.push(Cell::from_int(1));
  CHECK(stack.begin() != stack.end());
  CHECK(stack.begin() == stack.cbegin());
  CHECK(stack.end() == stack.cend());
}
