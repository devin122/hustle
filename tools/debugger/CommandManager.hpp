/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef COMMAND_MANAGER_HPP
#define COMMAND_MANAGER_HPP
#include "Command.hpp"
#include <map>
#include <memory>
#include <optional>
#include <string>
namespace hustle::debugger {

/***
 * Track set of commands which are registered to the debugger
 */
class CommandManager {
public:
  /***
   * Find a command which starts with the given prefix.
   *
   * \returns null if there are no matches, or multiple matches
   */
  Command* find_command(const std::string& name);

  /***
   * Add a command to this manager
   */
  void add_command(std::string name, std::unique_ptr<Command> cmd) {
    cmds_.emplace(std::move(name), std::move(cmd));
  }

private:
  std::map<std::string, std::unique_ptr<Command>> cmds_;
};

} // namespace hustle::debugger
#endif
