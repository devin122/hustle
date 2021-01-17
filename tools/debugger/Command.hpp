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

#ifndef COMMAND_HPP
#define COMMAND_HPP
#include <string>

namespace hustle {

struct VM;
namespace debugger {
class CommandManager;
class Command {
public:
  virtual ~Command(){};
  virtual void exec(VM*) = 0;

protected:
  Command(std::string name) : name_(std::move(name)){};

private:
  std::string name_;
  friend class CommandManager;
};

#define MAKE_CMD(cls_nm, nm)                                                   \
  class cls_nm : public Command {                                              \
  public:                                                                      \
    cls_nm() : Command(nm){};                                                  \
    void exec(VM*) override;                                                   \
  }

/// Print out the current huslte stack
MAKE_CMD(StackCommand, "stack");

/// Step into next instruction
MAKE_CMD(StepCommand, "step");

/// Step over next instruction
MAKE_CMD(OverCommand, "over");

/// Exit the debugger
MAKE_CMD(ExitCommand, "exit");

/// Print a backtrace of the current hustle call stack
MAKE_CMD(BackTraceCommand, "backtrace");

/// Resume execution
MAKE_CMD(ContinueCommand, "continue");

#undef MAKE_CMD
} // namespace debugger
} // namespace hustle
#endif
