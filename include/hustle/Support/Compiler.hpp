/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * \file
 * Provide misc support for detecting / adapting to different compilers
 */

#ifndef HUSTLE_SUPPORT_COMPILER_HPP
#define HUSTLE_SUPPORT_COMPILER_HPP

#if defined(_MSC_VER)
#define HUSTLE_NO_RETURN __declspec(noreturn)
#else
#define HUSTLE_NO_RETURN
#endif

#endif
