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
#include <inttypes.h>
#include <iostream>
#include <replxx.hxx>
#include <sstream>
#include <stdio.h>
#include <string>
#include <string_view>

using namespace hustle;
using namespace std::literals;

using Iterator = std::string_view::iterator;

const std::string HISTORY_FILE = "./hustle_history.txt"s;
using replxx::Replxx;
static void init_replxx(Replxx& rx) {
  rx.install_window_change_handler();
  rx.history_load(HISTORY_FILE);
  rx.set_max_history_size(128);
}

void shitty_repl(VM& vm) {
  while (true) {
    std::cout << "(bootstrap)> ";
    std::cout.flush();
    std::string line;
    std::getline(std::cin, line);

    std::string_view tokenize_span(line);
    try {
      auto it = tokenize_span.begin();
      auto tokens = bootstrap::tokenize(it, tokenize_span.end(), vm);

      for (auto cell : tokens) {
        if (cell == vm.globals.Exit) {
          std::cout << "Exiting bootstrap interpreter\n";
          return;
        }
        vm.evaluate(cell);
      }
      //        std::cout << "======\n";
      // dump_stack(vm, false);
      std::cout << "===== Stack: ====\n";
      print_stack(vm, 10);
      std::cout << std::endl;
    } catch (...) {
      std::cerr << "Parse error!" << std::endl;
    }
  }
}

void repl(VM& vm) {
  Replxx rx;
  init_replxx(rx);
  ReplxxStreamBuff buff(rx);
  HSTL_ASSERT(vm.lexer_.current_stream() == nullptr);
  // vm.lexer_.current_parser_ = &stream;
  vm.lexer_.add_stream(std::make_unique<std::istream>(&buff));
  auto* my_stream = vm.lexer_.current_stream();
  while (true) {

    buff.start();
    do {
      auto tok = vm.lexer_.token();
      // TODO flush the buffer on a lookup failure
      if (std::holds_alternative<intptr_t>(tok)) {
        vm.push(Cell::from_int(std::get<intptr_t>(tok)));
      } else if (std::holds_alternative<std::string>(tok)) {
        std::string& str = std::get<std::string>(tok);
        auto result = vm.lookup_symbol(str);
        vm.evaluate(Cell::from_raw(result));
      } else {
        // Occurs if we hit an end of stream
        // Kinda clunky, but just continue
        continue;
      }
    } while (vm.lexer_.current_stream() != my_stream);
    // TODO manage history
    // TODO skip whitespace
    buff.skip_space();
    if (buff.in_avail() == 0) {
      std::cout << "===== Stack: ====\n";
      print_stack(vm, 10);
      std::cout << std::endl;
    }
  }
}

int main(int argc, char** argv) {
  hustle::save_argv0(argv[0]);
  CLI::App app{"Hustle VM"};
  app.set_version_flag("-v,--version", HUSTLE_VERSION);

  bool no_kernel = false;
  bool old_repl = false;
  app.add_flag("--no-kernel", no_kernel, "Do not include the kernel");
  app.add_flag("--old-repl", old_repl, "Start up with the old repl");

  CLI11_PARSE(app, argc, argv);

  VM vm;
  if (!no_kernel) {
    vm.load_kernel();
  }
  // shitty_repl(vm);

  if (old_repl) {
    shitty_repl(vm);
  }
  repl(vm);

  return 0;
}
