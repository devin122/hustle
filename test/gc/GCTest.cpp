/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "hustle/GC.hpp"
#include "hustle/VM.hpp"
#include <catch2/catch_test_macros.hpp>
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
  Object o((ObjectTag)0, sizeof(Object));
  Object o2((ObjectTag)0, sizeof(Object));
  CHECK(!o.is_forwarding());
  o.forward_to(&o2);
  CHECK(o.is_forwarding());
  CHECK(o.get_forwarding() == &o2);
}
