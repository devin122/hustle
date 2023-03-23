/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * \file
 * Core definitions for the language
 */

#ifndef HUSTLE_CORE_HPP
#define HUSTLE_CORE_HPP

#include <stdint.h>

namespace hustle {

/**
 * Low level definition of our core Cell data type.
 *
 * This should only be used by low level code.
 * Most code should use  \ref Cell instead.
 *
 * \todo This should be moved into a private namespace at some point
 */
using cell_t = uintptr_t;

#include "cell_tags.def"

/// Number of bits used for pointer tagging
constexpr uintptr_t CELL_TAG_BITS = 3;

/// Mask value to get the pointer tag
constexpr uintptr_t CELL_TAG_MASK = (1 << CELL_TAG_BITS) - 1;
static_assert(CELL_TAG_MAX <= (1 << CELL_TAG_BITS), "Too many cell tag types");

// Limits on the magnitude of cell_t used to store int
constexpr intptr_t CELL_INT_MIN = INTPTR_MIN >> CELL_TAG_BITS;
constexpr intptr_t CELL_INT_MAX = INTPTR_MAX >> CELL_TAG_BITS;

// Basic sanity checks of our min/max values
static_assert(CELL_INT_MIN < 0);
static_assert(CELL_INT_MAX > 0);
} // namespace hustle

// GC safety annotations.
#if defined(HUSTLE_GC_SAFETY)
#define GC_SAFETY(x) __attribute((annotate("gc::" #x)))
#else
#define GC_SAFETY(x)
#endif

/// Mark a function as pontentially triggering a GC
#define HUSTLE_MAY_ALLOCATE GC_SAFETY(may_allocate)

/// Mark a class as being allocated on the GC'd heap
#define HUSTLE_HEAP_ALLOCATED GC_SAFETY(heap_allocated)

/// Marks a class as aliasing a heap pointer
#define HUSTLE_HEAP_POINTER GC_SAFETY(heap_pointer)

#endif
