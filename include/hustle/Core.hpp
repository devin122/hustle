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
