/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Command.hpp"
#include "CommandManager.hpp"
#include "hustle/Object.hpp"
#include "hustle/Parser/BootstrapLexer.hpp"
#include "hustle/Parser/Lexer.hpp"
#include "hustle/Support/Assert.hpp"
#include "hustle/Support/IndentingStream.hpp"
#include "hustle/Support/Utility.hpp"
#include "hustle/VM.hpp"
#include "hustle/config.h"
#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <functional>
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

// TODO: remove this once code is ported
// using string_span = gsl::string_span<gsl::dynamic_extent>;
using string_span = std::string_view;

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
  vm.set_debug_listener(bp_handler);
  vm.load_kernel();

  // shitty_repl(vm);
  vm.lexer_.add_stream(std::make_unique<std::ifstream>(input_file));
  run_test(vm);

  return 0;
}
