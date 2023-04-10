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

#include <stdio.h>

#include <Windows.h>

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

static DWORD native_flags(unsigned flags) {
  DWORD protection = PAGE_NOACCESS;
  if (flags & Memory::MEM_WRITE) {
    HSTL_ASSERT(flags & Memory::MEM_READ);
    if (flags & Memory::MEM_EXEC) {
      protection = PAGE_EXECUTE_READWRITE;
    } else {
      protection = PAGE_READWRITE;
    }
  } else if (flags & Memory::MEM_EXEC) {
    if (flags & Memory::MEM_READ) {
      protection = PAGE_EXECUTE_READ;
    } else {
      protection = PAGE_EXECUTE;
    }
  } else if (flags & Memory::MEM_READ) {
    protection = PAGE_READONLY;
  }
  return protection;
}

Expected<MemorySegment> Memory::allocate(size_t size, unsigned flags) {

  DWORD protection = native_flags(flags);
  // TODO round up to page size;
  void* memory =
      VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, protection);
  if (memory == (void*)-1) {
    return -1; // TODO need real error
  }
  return MemorySegment(memory, size);
  // return {nullptr, 0};
}

void Memory::release(MemorySegment& segment) {
  if (segment.base() == nullptr) {
    return;
  }

  if (!VirtualFree(segment.base(), 0, MEM_RELEASE)) {
    DWORD dw = GetLastError();
    char* lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&lpMsgBuf, 0, NULL);
    puts(lpMsgBuf);
    HSTL_ASSERT(false);
  }
  segment.base_ = nullptr;
  segment.size_ = 0;
}

void Memory::protect(void* addr, size_t size, unsigned flags) {
  DWORD old_flags = 0;
  if (!VirtualProtect(addr, size, native_flags(flags), &old_flags)) {
    // TODO: better integrate this error
    DWORD dw = GetLastError();
    char* lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&lpMsgBuf, 0, NULL);
    puts(lpMsgBuf);
    HSTL_ASSERT(false);
  }
}
