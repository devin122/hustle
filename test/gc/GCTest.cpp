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
#include <hustle/GC.hpp>
#include <hustle/VM.hpp>
#include <iterator>

using namespace hustle;
TEST_CASE("Bogus", "[gc]") { CHECK(1 == 1); }

TEST_CASE("HeapRegion", "[gc][region]") {
  HeapRegion region(nullptr);

  auto start_size = region.bytes_free();

  void* alloc1 = region.allocate(32);
  CHECK(alloc1 != nullptr);
  CHECK(region.contains(alloc1));
  CHECK(region.bytes_free() <= (start_size - 32));

  CHECK(!region.contains(&start_size));
}

TEST_CASE("HeapTest", "[gc]") {
  Cell cells[4];
  auto mark_fn = [&](Heap::MarkFunction fn) {
    fn((cell_t*)&cells[0]);
    // fn((cell_t*)cells[1]);
  };

  Heap heap(mark_fn);
  // Make bogus allocation
  heap.allocate(32);

  const char test_string[] = "abc";
  auto* raw_ptr = new (heap.allocate(sizeof(String) + sizeof(test_string)))
      String(test_string, sizeof(test_string));

  {
    std::string_view sv = *raw_ptr;
    CHECK(std::equal(sv.begin(), sv.end(), test_string,
                     test_string + sizeof(test_string)));
  }

  cells[0] = raw_ptr;
  heap.gc();
  CHECK(cells[0].cast<String>() != raw_ptr);

  {
    std::string_view sv = *cells[0].cast<String>();
    CHECK(std::equal(sv.begin(), sv.end(), test_string,
                     test_string + sizeof(test_string)));
  }
}

TEST_CASE("HandleTest", "[handle]") {
  // Test basic handle functionality
  HandleManager mgr;

  auto mark_fn = [&](Heap::MarkFunction fn) { mgr.mark_handles(fn); };
  Heap heap(mark_fn);

  const char test_string[] = "abc";
  auto* raw_ptr = new (heap.allocate(sizeof(String) + sizeof(test_string)))
      String(test_string, sizeof(test_string));

  Handle<String> str_handle = mgr.make_handle<String>(raw_ptr);

  {
    // Sanity check to make sure the string was allocated properly;
    std::string_view sv = *raw_ptr;
    CHECK(std::equal(sv.begin(), sv.end(), test_string,
                     test_string + sizeof(test_string)));
  }

  heap.gc();
  CHECK((String*)str_handle != raw_ptr);

  {
    std::string_view sv = *str_handle;
    CHECK(std::equal(sv.begin(), sv.end(), test_string,
                     test_string + sizeof(test_string)));
  }
}

TEST_CASE("ObjectForwarding") {
  Object o((cell_tag)0, sizeof(Object));
  Object o2((cell_tag)0, sizeof(Object));
  CHECK(!o.is_forwarding());
  o.forward_to(&o2);
  CHECK(o.is_forwarding());
  CHECK(o.get_forwarding() == &o2);
}
