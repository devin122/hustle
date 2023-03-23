/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * \file
 * Assertion macro and helpers
 */
#ifndef HUSTLE_SUPPORT_ASSERT_HPP
#define HUSTLE_SUPPORT_ASSERT_HPP

#include <assert.h>

namespace hustle {

// void
[[noreturn]] void assertion_failure();
} // namespace hustle

/**
 * Simple assertion macro.
 *
 * Currently simply does a c assert(), but eventually this should have
 * additional logic
 */
#define HSTL_ASSERT(EXPR) assert(EXPR)

#endif
