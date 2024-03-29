/*
 * Copyright (c) 2021, Devin Nakamura
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

// OSX throws a SIGBUS rather than a SIGSEGV
#if defined(__APPLE__)
  constexpr int SIGNAL_NUM = SIGBUS;
#else
  constexpr int SIGNAL_NUM = SIGSEGV;
#endif

  SignalHandlerPointer previous_handler;
  addr = segment.base();
  size = segment.size();
  previous_handler = signal(SIGNAL_NUM, [](int signal) {
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
