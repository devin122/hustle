/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Command.hpp"
#include <fmt/core.h>
#include <hustle/Support/Utility.hpp>
#include <hustle/VM.hpp>
#include <stdlib.h>
using namespace hustle::debugger;
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
