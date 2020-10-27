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

#ifndef HUSTLE_GC_HPP
#define HUSTLE_GC_HPP

#include <stddef.h>
#include <stdint.h>

namespace hustle {

struct Object;
struct VM;

/*
class Heap {

public:
    struct FreeObject;
    Heap(VM *vm);
    ~Heap();
    // TODO: this should be inlined for perf, but for the moment we leave it
this way for flexibility Object* allocate(size_t size); void gc(); private: void
mark(); void mark_free_objects(); void sweep(); void clear(); //TODO this can be
optimized out

    //Its lame, but at the moment we have a fixed size heap
    VM* const vm_;
    static constexpr size_t HEAP_SIZE = 16 * 1024 * 1024;
    void *memory_block_ = nullptr;
    FreeObject *free_obj_;
    size_t free_bytes_;
    bool has_marked = false;
};
*/

class Heap;

class HeapRegion {
  friend class Heap;
  static constexpr size_t REGION_SIZE = 16 * 1024 * 1024;

public:
  HeapRegion(Heap* heap);
  ~HeapRegion();
  Object* allocate(size_t sz);

  size_t bytes_free() { return end_ - allocate_ptr_; }

  bool contains(void* ptr) const noexcept {
    return ptr >= start_ && ptr < end_;
  }

private:
  void reset();
  Heap* const heap_;
  uint8_t* start_;
  uint8_t* allocate_ptr_;
  uint8_t* end_;
};

class Heap {

public:
  struct FreeObject;
  Heap(VM* vm);
  ~Heap() = default;
  // TODO: this should be inlined for perf, but for the moment we leave it this
  // way for flexibility
  Object* allocate(size_t size);
  void gc();

private:
  void swap_heaps();
  VM* vm_;
  HeapRegion region_a_, region_b_;
  HeapRegion *current_heap_, *backup_heap_;
};

} // namespace hustle

#endif
