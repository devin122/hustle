/*
 * Copyright (c) 2021, Devin Nakamura
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

#ifndef HUSTLE_SUPPORT_MEMORY_HPP
#define HUSTLE_SUPPORT_MEMORY_HPP
#include "hustle/Support/Utility.hpp"
namespace hustle {
class MemorySegment;

class Memory {
public:
  enum Flags { MEM_READ = 1, MEM_WRITE = 2, MEM_EXEC = 4 };

  static MemorySegment allocate(size_t size, unsigned flags);

  // TODO: do we want to be able to change protections on a subsection?
  static void protect(MemorySegment& segment, unsigned flags);

  // Hack for testing
  static void protect(void* addr, size_t sz, unsigned flags);

  // Should not need to call this directly
  static void release(MemorySegment& segment);
};

class MemorySegment {
public:
  MemorySegment(const MemorySegment&) = delete;
  MemorySegment& operator=(const MemorySegment&) = delete;

  MemorySegment(MemorySegment&& other)
      : base_(other.base_), size_(other.size_) {
    other.base_ = nullptr;
    other.size_ = 0;
  }

  ~MemorySegment() { Memory::release(*this); }

  bool contains(void* addr) const {
    return (addr >= base_) && (addr <= pointer_add<void>(base_, size_));
  }

  void* base() const { return base_; }
  size_t size() const { return size_; }

private:
  MemorySegment(void* addr, size_t size) : base_(addr), size_(size) {}
  void* base_ = nullptr;
  size_t size_ = 0;

  friend class Memory;
};

inline void Memory::protect(MemorySegment& segment, unsigned flags) {
  protect(segment.base(), segment.size(), flags);
}
} // namespace hustle
#endif
