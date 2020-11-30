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
