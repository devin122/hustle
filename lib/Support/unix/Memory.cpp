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

#include "hustle/Support/Memory.hpp"
#include "hustle/Support/Assert.hpp"
#include <sys/mman.h>

using namespace hustle;
/*
class Memory {
public:
  enum Flags { MEM_READ = 1, MEM_WRITE = 2, MEM_EXEC = 4 };

  static MemorySegment allocate(size_t size, unsigned flags);

  static void protect(MemorySegment& segment, unsigned flags);

  // Should not need to call this directly
  static void release(MemorySegment& segment);
};*/

static int native_protection_flags(unsigned flags) {
  int prot = 0;
  if (flags & Memory::MEM_READ)
    prot |= PROT_READ;
  if (flags & Memory::MEM_WRITE)
    prot |= PROT_WRITE;
  if (flags & Memory::MEM_EXEC)
    prot |= PROT_EXEC;
  return prot;
}

MemorySegment Memory::allocate(size_t size, unsigned flags) {
  const int prot = native_protection_flags(flags);

  // TODO round up to a page size;
  void* addr = mmap(nullptr, size, prot, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  HSTL_ASSERT(addr != (void*)-1);
  return MemorySegment(addr, size);
}

void Memory::release(MemorySegment& segment) {
  if (segment.base() == nullptr) {
    return;
  }
  int rc = munmap(segment.base(), segment.size());
  HSTL_ASSERT(rc == 0);
  segment.base_ = nullptr;
  segment.size_ = 0;
}

void Memory::protect(void* addr, size_t size, unsigned flags) {
  // TODO: should be performing sanity checks on passed values.
  int rc = mprotect(addr, size, native_protection_flags(flags));
  HSTL_ASSERT(rc == 0);
}
