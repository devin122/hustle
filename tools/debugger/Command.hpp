/*
 * Copyright (c) 2020, Devin Nakamura
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
