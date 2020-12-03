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

#include "Command.hpp"
#include <fmt/core.h>
#include <hustle/Utility.hpp>
#include <hustle/VM.hpp>
#include <stdlib.h>
using namespace hustle;
using namespace std::literals;

namespace hustle {
struct DebuggerInterface {
  volatile char code = 0;
  enum State { DBG_NONE, DBG_STEP, DBG_OVER } state;
  const void* call_stack = nullptr;
};
extern DebuggerInterface dbg_interface;
} // namespace hustle

void ExitCommand::exec(VM*) { exit(0); }

void StackCommand::exec(VM* vm) { dump_stack(*vm, true); }

void StepCommand::exec(VM* vm) { dbg_interface.code = 's'; }

void OverCommand::exec(VM* vm) { dbg_interface.code = 'o'; }

void BackTraceCommand::exec(VM* vm) {
  fmt::print("======= Backtrace ======\n");
  const auto* end = vm->call_stack_.end();
  for (auto* frame = vm->call_stack_.begin(); frame < end; ++frame) {
    std::string_view word_name;
    if (frame->word.is_a<Word>()) {
      word_name = *cast<Word>(frame->word)->name;
    } else {
      word_name = "<Anonymous>"sv;
    }
    fmt::print("{}+{}\n", word_name, cast<intptr_t>(frame->offset));
  }
}

void ContinueCommand::exec(VM* vm) {
  // no-op
}
