/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#if defined(_WIN32)
#include <Windows.h>
#include <debugapi.h>
#endif

namespace hustle {

void debug_break() {
#if defined(_WIN32)
  DebugBreak();
#endif
}

} // namespace hustle
