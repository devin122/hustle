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

#include <catch2/catch.hpp>
#include <signal.h>
#include <stdio.h>
#ifdef _WIN32

#include <Windows.h>
#endif
using namespace hustle;

// TODO this is a gross hack
void* addr = nullptr;
size_t size = 0;
static bool handler_triggered = false;

#ifdef _WIN32

PVOID handler_token = nullptr;

static LONG fault_handler(_EXCEPTION_POINTERS* ExceptionInfo) {
  if (ExceptionInfo->ExceptionRecord->ExceptionCode !=
      EXCEPTION_ACCESS_VIOLATION) {
    return EXCEPTION_CONTINUE_SEARCH;
  }
  // Memory::protect(addr, size, new_flags);
  RemoveVectoredExceptionHandler(handler_token);
  return EXCEPTION_CONTINUE_EXECUTION;
}
template <unsigned new_flags, typename T>
void seg_protect(MemorySegment& segment, T body) {
  // Ideally this would just use a _try, but that has lower priority than
  // catch2's vector handler
  addr = segment.base();
  size = segment.size();
  handler_token =
      AddVectoredExceptionHandler(1, [](_EXCEPTION_POINTERS* ExceptionInfo) {
        if (ExceptionInfo->ExceptionRecord->ExceptionCode !=
            EXCEPTION_ACCESS_VIOLATION) {
          return (LONG)EXCEPTION_CONTINUE_SEARCH;
        }
        handler_triggered = true;
        Memory::protect(addr, size, new_flags);
        RemoveVectoredExceptionHandler(handler_token);
        return (LONG)EXCEPTION_CONTINUE_EXECUTION;
      });
  body();
}
#else
typedef void (*SignalHandlerPointer)(int);
template <unsigned new_flags, typename T>
void seg_protect(MemorySegment& segment, T body) {

  SignalHandlerPointer previous_handler;
  addr = segment.base();
  size = segment.size();
  previous_handler = signal(SIGSEGV, [](int signal) {
    handler_triggered = true;
    Memory::protect(addr, size, new_flags);
  });
  body();

  signal(SIGSEGV, previous_handler);
}
#endif

TEST_CASE("Basic allocation test", "[memory]") {
  constexpr size_t ALLOCATION_SIZE = 10 * 4 * 1024;
  auto segment = Memory::allocate(ALLOCATION_SIZE, Memory::MEM_READ);
  CHECK(segment.base() != nullptr);
  // CHECK(segment.base() != (void*) -1);
  CHECK(segment.size() >= ALLOCATION_SIZE);
  // Try writing into the allocated memory
  printf("DEBUG: mem addr = %p\n", segment.base());
  int* data = (int*)segment.base();

  // Reading should be ok
  (void)data[0];

  seg_protect<Memory::MEM_READ | Memory::MEM_WRITE>(segment,
                                                    [=] { data[0] = 1; });
  CHECK(handler_triggered == true);
}
