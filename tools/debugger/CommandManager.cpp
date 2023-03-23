/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "CommandManager.hpp"

#include <limits>

using namespace hustle::debugger;

Command* CommandManager::find_command(const std::string& name) {
  // TODO: this doesn't seem locale safe
  constexpr auto char_max = std::numeric_limits<char>::max();

  // Extra allocation + copy here seems gross
  std::string first_after = name + char_max;

  auto begin_it = cmds_.lower_bound(name);
  auto end_it = cmds_.lower_bound(first_after);

  if (begin_it == cmds_.end()) {
    return nullptr;
  }

  if (begin_it->first == name) {
    return begin_it->second.get();
  }

  auto it = begin_it;
  if (++it == end_it) {
    // We have only 1 partial match
    return begin_it->second.get();
  } else {
    // multiple matches
    return nullptr;
  }
  return nullptr;
}
