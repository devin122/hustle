/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef HUSTLE_VM_CONTEXT_HPP
#define HUSTLE_VM_CONTEXT_HPP

namespace hustle {
struct VM;
struct Stack;
struct CallStack;

struct Context {
  VM* vm;
  Stack* stack;
  CallStack* CallStack;
};

} // namespace hustle
#endif
