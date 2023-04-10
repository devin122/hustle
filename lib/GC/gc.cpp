/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "hustle/GC.hpp"
#include "hustle/Object.hpp"
#include "hustle/VM.hpp"

#include <assert.h>
#include <stack>
#include <stdio.h>
#include <stdlib.h>

using namespace hustle;

hustle::HeapRegion::HeapRegion(Heap* heap) : heap_(heap) {
  start_ = (uint8_t*)malloc(REGION_SIZE);
  HSTL_ASSERT(start_ != nullptr);
  end_ = start_ + REGION_SIZE;
  allocate_ptr_ = start_;
  HSTL_ASSERT((((uintptr_t)start_) & CELL_TAG_MASK) == 0);
  reset();
}

HeapRegion::~HeapRegion() {
  if (start_ != nullptr)
    ::free(start_);
}

Object* HeapRegion::allocate(size_t sz) {
  constexpr size_t object_align = 1 << CELL_TAG_BITS;
  sz = ((sz + object_align - 1) / object_align) * object_align;

  HSTL_ASSERT((sz & (object_align - 1)) == 0);

  HSTL_ASSERT(sz < bytes_free());
  // assert((uintpt))
  auto new_ptr = (Object*)allocate_ptr_;
  allocate_ptr_ += sz;
  HSTL_ASSERT(allocate_ptr_ < end_);
  return new_ptr;
}

void HeapRegion::reset() {
  allocate_ptr_ = start_;
  memset(start_, 0, end_ - start_);
}

Heap::Heap(MarkRootsFunction mark_roots)
    : mark_roots_(mark_roots), region_a_(this), region_b_(this),
      current_heap_(&region_a_), backup_heap_(&region_b_) {}

Object* Heap::allocate(size_t sz) {

  // Force a gc for testing
  if (debug_alloc) {
    swap_heaps();
  }
  // TODO this is dumb

  if (current_heap_->bytes_free() < 1024 * 1024 ||
      current_heap_->bytes_free() < sz) {
    swap_heaps();
  }
  HSTL_ASSERT(current_heap_->bytes_free() > sz);
  return current_heap_->allocate(sz);
}

void Heap::gc() { swap_heaps(); }

void Heap::swap_heaps() {
  HSTL_ASSERT(!running_gc_);
  running_gc_ = true;
  std::stack<Object*> work_stack;

  auto copy_object = [&, current_heap = current_heap_,
                      backup_heap = backup_heap_](cell_t* handle) {
    if (is_a<Object>(*handle)) {
      Object* obj = cell_helpers::get_cell_pointer(*handle);
      if (current_heap->contains(obj)) {
        if (obj->is_forwarding()) {
          Object* forwarded = obj->get_forwarding();
          *handle = forwarded->get_cell().raw();
          return;
        } else {
          auto sz = obj->size();
          Object* new_ptr = backup_heap->allocate(sz);
          memcpy(new_ptr, obj, sz);
          obj->forward_to(new_ptr);
          work_stack.push(new_ptr);
          *handle = new_ptr->get_cell().raw();
        }
      }
    }
  };

  // fixup the roots
  mark_roots_(copy_object);

  while (!work_stack.empty()) {
    Object* o = work_stack.top();
    work_stack.pop();
    auto type = o->tag();
    switch (type) {
    case OBJ_TAG_ARRAY: {
      Array* array = (Array*)o;
      Cell* end = array->end();
      for (Cell* element = array->begin(); element < end; ++element) {
        copy_object((cell_t*)element); // TODO this is a hack
      }
    } break;
    case OBJ_TAG_STRING:
      break;
    case OBJ_TAG_QUOTE: {
      Quotation* quote = (Quotation*)(o);
      if (quote->definition != nullptr) {
        copy_object((cell_t*)&quote->definition);
      }
      break;
    }
    case OBJ_TAG_WRAPPER: {
      Wrapper* wrapper = (Wrapper*)(o);
      copy_object((cell_t*)&wrapper->wrapped);
    } break;

    case OBJ_TAG_WORD: {
      Word* word = (Word*)o;
      copy_object((cell_t*)&word->name);
      copy_object((cell_t*)&word->definition);
      copy_object((cell_t*)&word->properties);
    } break;
    default:
      HSTL_ASSERT(false);
    }
  }

  current_heap_->reset();
  auto tmp = current_heap_;
  current_heap_ = backup_heap_;
  backup_heap_ = tmp;

  running_gc_ = false;
}

/*
void Heap::mark() {
        std::stack<Object*> work_stack;
        auto mark_object = [&](Object* o) {
                if (!(o->header.marked & HEADER_MARK_BIT)) {
                        o->mark();
                        work_stack.push(o);
                }
        };

        assert(vm_ != nullptr);
        vm_->mark_roots(mark_object);

        while (!work_stack.empty()) {
                Object* o = work_stack.top();
                work_stack.pop();

                cell_tag type =o->tag();
                switch (type) {
                case CELL_ARRAY:
                        for (cell_t element : *((Array*)o)) {
                                if (is_a<Object>(element)) {
                                        mark_object(get_cell_pointer(element));
                                }
                        }
                        break;
                case CELL_STRING:
                        break;
                case CELL_QUOTE: {
                        Quotation* quote = (Quotation*)(o);
                        if (quote->definition != nullptr) {
                                mark_object(quote->definition);
                        }
                        break;
                }
                default:
                        assert(false);
                }
        }
}
*/
void HandleManager::mark_handles(Heap::MarkFunction mark_fn) {
  auto* handle = root_handle_.next_;
  while (handle != &root_handle_) {
    cell_t* cell_ptr = (cell_t*)&handle->cell_;
    mark_fn(cell_ptr);
    HSTL_ASSERT(handle->next_ != handle);
    handle = handle->next_;
  }
}
