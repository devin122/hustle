/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef HUSTLE_GC_HPP
#define HUSTLE_GC_HPP

#include "hustle/Core.hpp"
#include "hustle/Support/Assert.hpp"
#include "hustle/VM/Cell.hpp"
#include <functional>
#include <stddef.h>
#include <stdint.h>

namespace hustle {

struct Object;
struct VM;

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
  using MarkFunction = std::function<void(cell_t*)>;
  using MarkRootsFunction = std::function<void(MarkFunction)>;
  struct FreeObject;
  Heap(MarkRootsFunction);
  ~Heap() = default;
  // TODO: this should be inlined for perf, but for the moment we leave it this
  // way for flexibility
  bool debug_alloc = false;
  Object* allocate(size_t size) HUSTLE_MAY_ALLOCATE;
  void gc();

private:
  void swap_heaps();
  MarkRootsFunction mark_roots_;
  HeapRegion region_a_, region_b_;
  HeapRegion *current_heap_, *backup_heap_;
  bool running_gc_ = false;
};

class HandleManager;

/**
 * Common functionality of registering handles to be marked.
 *
 * This is exposed via Handle or TypedHandle.
 */
class HandleBase {
  HandleBase* prev_;
  HandleBase* next_;

protected:
  Cell cell_ = Cell::from_raw(0);

  HandleBase() : prev_(this), next_(this) {}
  // Insert after
  HandleBase(HandleBase* before, Cell c) : cell_(c) {
    HSTL_ASSERT(before != nullptr);
    HSTL_ASSERT(before->next_ != nullptr);
    HSTL_ASSERT(before->prev_ != nullptr);
    prev_ = before;
    next_ = before->next_;
    HSTL_ASSERT(next_->prev_ == before);
    next_->prev_ = this;
    before->next_ = this;
  }

public:
  HandleBase(const HandleBase&) = delete;
  HandleBase(const HandleBase&&) = delete;
  ~HandleBase() {
    // unlink ourself from the list
    if (prev_) {
      HSTL_ASSERT(prev_->next_ == this);
      prev_->next_ = next_;
    }
    if (next_) {
      HSTL_ASSERT(next_->prev_ == this);
      next_->prev_ = prev_;
    }
  }

  Cell cell() { return cell_; }

  bool operator==(const HandleBase& other) const {
    return cell_ == other.cell_;
  }

  HandleBase& operator=(const HandleBase&) = delete;
  friend class HandleManager;
};

template <typename T = Object>
class Handle : public HandleBase {
public:
  explicit Handle(HandleBase* before, T* ptr) : HandleBase(before, ptr) {}
  // TODO: should this be something more explicit?
  // I'm worried we might silently generate a bunch of conversions when we dont
  // want to
  operator T*() {
    if (cell_ == Cell::from_raw(0)) {
      return nullptr;
    }
    return cell_.cast<T>();
  }

  T* operator->() {
    HSTL_ASSERT(cell_ != Cell::from_raw(0));
    T* ptr = cell_.cast<T>();
    HSTL_ASSERT(ptr != nullptr);
    return ptr;
  }
};

template <typename T>
inline cell_t make_cell(Handle<T>& handle) {
  return cell_helpers::make_cell((T*)handle);
}

class HandleManager {

  // Bogus handle which is the root of our circular linked list
  HandleBase root_handle_;

public:
  HandleManager() = default;
  void mark_handles(Heap::MarkFunction mark_fn);

  template <typename T>
  Handle<T> make_handle(T* ptr) {
    return Handle<T>(&root_handle_, ptr);
  }

  HandleBase make_handle(Cell c) { return HandleBase(&root_handle_, c); }

  // Handle make_handle(Cell cell) { return Handle(&root_handle_, cell); }
};
} // namespace hustle

#endif
