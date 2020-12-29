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
#include <hustle/Support/Assert.hpp>
#include <hustle/Support/IndentingStream.hpp>
#include <hustle/Support/Utility.hpp>
#include <hustle/VM.hpp>
#include <hustle/config.h>

#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <chrono>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fstream>
#include <gsl/string_span>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

using namespace hustle;
using namespace std::literals;
using string_span = gsl::string_span<gsl::dynamic_extent>;

void register_primitives(VM& vm);

static void load_kernel(VM& vm) {
  auto path = hustle::hustle_lib_dir().append("literals.hsl");
  std::ifstream kernel(hustle::hustle_lib_dir() / "literals.hsl");

  if (!kernel) {
    std::cerr << "Failed to load kernel\n";
    abort();
  }

  std::string line;
  while (!kernel.eof()) {
    HSTL_ASSERT(!kernel.fail());
    HSTL_ASSERT(!kernel.bad());
    line = ""s;
    std::getline(kernel, line);
    string_span tokenize_span(line);
    auto it = tokenize_span.begin();
    auto tokens = bootstrap::tokenize(it, tokenize_span.end(), vm);
    for (auto cell : tokens) {
      vm.evaluate(cell);
    }
  }
}

static int tests_run = 0;
static int tests_passed = 0;

static void check_handler(VM* vm, Quotation*) {
  // TODO should be a handle
  auto expected_stack_handle =
      vm->make_handle<Array>(cast<Array>(vm->stack_.pop()));
  ++tests_run;

  vm->call(vm->pop());

  Array* expected_stack = expected_stack_handle;
  bool eq = std::equal(expected_stack->begin(), expected_stack->end(),
                       std::make_reverse_iterator(vm->stack_.end()),
                       std::make_reverse_iterator(vm->stack_.begin()));
  if (eq) {
    ++tests_passed;
  } else {
    fmt::print("Test failure in test #{}\n", tests_run);
  }
  vm->stack_.clear();
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

static void write_xml(const std::string& fname,
                      const std::string& suite_name = "unknown"s,
                      bool bogus = false) {
  std::ofstream fs(fname);
  IndentingStream os(fs);

  int duration = 1;
  int failures = bogus ? 1 : tests_run - tests_passed;
  int num_tests = bogus ? 1 : tests_run;

  os.writeln("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
  os.writeln("<testsuites>");
  os.indent();
  os.writeln(
      "<testsuite name='hustle-test' errors='0' failures='{}' tests='{}' "
      "hostname='tbd' time='{}' timestamp='{:%Y-%m-%dT%H:%M:%SZ}'>",
      failures, num_tests, duration, fmt::gmtime(std::time(NULL)));
  os.indent();
  os.writeln("<testcase classname='hustle-test.{}' name='{}' time='1'/>",
             suite_name, suite_name);
  os.writeln("<system-out/>");
  os.writeln("<system-err/>");

  os.outdent();
  os.writeln("</testsuite>");

  os.outdent();
  os.writeln("</testsuites>");
  fs.close();
}

int main(int argc, char** argv) {
  hustle::save_argv0(argv[0]);
  std::string input_file;
  std::string xml_out;
  std::string suite_name;
  CLI::App app{"hustle-test"};
  app.add_option("test_suite", input_file, "Input test suite to run")
      ->check(CLI::ExistingFile)
      ->required(true);
  auto name_option = app.add_option("--name", suite_name, "Name of test suite");

  app.add_option("--xml", xml_out, "File to output xml results")
      ->needs(name_option);

  CLI11_PARSE(app, argc, argv);

  if (xml_out != "") {
    write_xml(xml_out, suite_name, true);
  }

  VM vm;
  register_primitives(vm);
  load_kernel(vm);

  vm.heap_.debug_alloc = true;
  vm.register_primitive("check", check_handler);

  // std::ifstream fstream(input_file);
  vm.lexer_.add_stream(std::make_unique<std::ifstream>(input_file));
  run_test(vm);

  fmt::print("Passed {} out of {} tests\n", tests_passed, tests_run);
  if (tests_passed != tests_run) {
    fmt::print("FAILED {} tests\n", tests_run - tests_passed);
  }
  if (xml_out != "") {
    write_xml(xml_out, suite_name);
  }

  if (tests_passed != tests_run) {
    return 1;
  }
  return 0;
}
