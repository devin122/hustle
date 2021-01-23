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
#include "CommandManager.hpp"

#include <hustle/Object.hpp>
#include <hustle/Parser/BootstrapLexer.hpp>
#include <hustle/Parser/Lexer.hpp>
#include <hustle/Support/Assert.hpp>
#include <hustle/Support/IndentingStream.hpp>
#include <hustle/Support/Utility.hpp>
#include <hustle/VM.hpp>
#include <hustle/config.h>

#include "CLI/App.hpp"
#include "CLI/Config.hpp"
#include "CLI/Formatter.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <functional>
#include <gsl/string_span>
#include <inttypes.h>
#include <iostream>
#include <map>
#include <replxx.hxx>
#include <sstream>
#include <stdio.h>
#include <string>

using namespace hustle::debugger;
using namespace hustle;
using namespace std::literals;
using replxx::Replxx;

Replxx rx;
void register_primitives(VM& vm);
// hack
namespace hustle {

struct DebuggerInterface {
  volatile char code = 0;
  enum State { DBG_NONE, DBG_STEP, DBG_OVER } state;
  const void* call_stack = nullptr;
};
VM* global_vm; // HACK
extern DebuggerInterface dbg_interface;
} // namespace hustle

using string_span = gsl::string_span<gsl::dynamic_extent>;
// using string_span = std::string_view;

using Iterator = string_span::iterator;

// TODO should make line const
const std::string HISTORY_FILE = "./hustle_history.txt"s;

CommandManager cmd_mgr;

static void init_replxx(Replxx& rx) {
  rx.install_window_change_handler();
  rx.history_load(HISTORY_FILE);
  rx.set_max_history_size(128);
}

static void init_cmds() {
  cmd_mgr.add_command("backtrace", std::make_unique<BackTraceCommand>());
  cmd_mgr.add_command("stack", std::make_unique<StackCommand>());
  cmd_mgr.add_command("exit", std::make_unique<ExitCommand>());
  cmd_mgr.add_command("step", std::make_unique<StepCommand>());
  cmd_mgr.add_command("over", std::make_unique<OverCommand>());

  // TODO hack
  cmd_mgr.add_command("s", std::make_unique<StepCommand>());
}

static void bp_handler() {

  while (true) {
    const char* line = nullptr;
    do {
      line = rx.input("(hdb)> ");
    } while ((line == nullptr) && (errno == EAGAIN));
    auto cmd = cmd_mgr.find_command(line);
    if (cmd != nullptr) {
      cmd->exec(global_vm);
    } else {
      std::cout << "Could not find matching command\n";
    }
  }
}

static void run_test(VM& vm) {
  do {
    auto tok = vm.lexer_.token();
    if (std::holds_alternative<intptr_t>(tok)) {
      vm.push(Cell::from_int(std::get<intptr_t>(tok)));
    } else if (std::holds_alternative<std::string>(tok)) {
      std::string& str = std::get<std::string>(tok);
      auto result = vm.lookup_symbol(str);
      vm.evaluate(Cell::from_raw(result));
    } else {
      // Shouldnt be possible
      if (vm.lexer_.current_stream() == nullptr) {
        break;
      }
      HSTL_ASSERT(false);
    }
  } while (vm.lexer_.current_stream() != nullptr);
}

int main(int argc, char** argv) {

  std::string input_file;
  hustle::save_argv0(argv[0]);
  CLI::App app{"Hustle Debugger test"};
  app.set_version_flag("-v,--version", HUSTLE_VERSION);
  app.add_option("program", input_file, "program to run")
      ->check(CLI::ExistingFile)
      ->required(true);
  CLI11_PARSE(app, argc, argv);

  init_replxx(rx);
  init_cmds();

  VM vm;
  global_vm = &vm;
  register_primitives(vm);
  vm.set_debug_listener(bp_handler);
  vm.load_kernel();

  // shitty_repl(vm);
  vm.lexer_.add_stream(std::make_unique<std::ifstream>(input_file));
  run_test(vm);

  return 0;
}
