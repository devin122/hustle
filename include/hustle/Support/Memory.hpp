/*
 * Copyright (c) 2021, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef HUSTLE_SUPPORT_MEMORY_HPP
#define HUSTLE_SUPPORT_MEMORY_HPP
#include "hustle/Support/Error.hpp"
#include "hustle/Support/Utility.hpp"
namespace hustle {
class MemorySegment;

/**
 *  Used as a namespace for various memory functions.
 *  Implemented as a class so that it can be friends with MemorySegment.
 */
class Memory {
public:
  enum Flags { MEM_READ = 1, MEM_WRITE = 2, MEM_EXEC = 4 };

  static Expected<MemorySegment> allocate(size_t size, unsigned flags);

  // TODO: do we want to be able to change protections on a subsection?
  static void protect(MemorySegment& segment, unsigned flags);

  // Hack for testing
  static void protect(void* addr, size_t sz, unsigned flags);

  // Should not need to call this directly
  static void release(MemorySegment& segment);
};

/**
 * Acts as a handle to contigous memory, and implements RAII.
 */
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
