/*
 * Copyright (c) 2020, Devin Nakamura
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
#include <fmt/chrono.h>
#include <fmt/core.h>

#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

using namespace hustle;
using namespace std::literals;

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

  // TODO we should really be making 1 entry for each test case
  if (failures == 0) {
    os.writeln("<testcase classname='hustle-test.{}' name='{}' time='1'/>",
               suite_name, suite_name);
  } else {
    os.writeln("<testcase classname='hustle-test.{}' name='{}' time='1'>",
               suite_name, suite_name);
    os.indent();
    os.writeln("<failure>").indent();
    os.writeln("dummy failure message").outdent();
    os.writeln("</failure>").outdent();
    os.writeln("</testcase>");
  }

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
  vm.load_kernel();

  vm.heap_.debug_alloc = true;
  vm.register_primitive("check", check_handler);

  // std::ifstream fstream(input_file);
  vm.lexer_.add_stream(std::make_unique<std::ifstream>(input_file));

  try {
    run_test(vm);
  } catch (std::exception& e) {
    // bogus code to make sure that we have at least 1 failed test
    if (tests_passed >= tests_run) {
      tests_run = tests_passed + 1;
    }
    fmt::print("Exception thrown {}\n", e.what());

    // Generate some debug info
    fmt::print("======= Stack ========\n");
    hustle::print_stack(vm, 3);

    fmt::print("======= Backtrace ======\n");
    // TODO: this code is lifted from the primitive, should be factored
    // somewhere common
    fmt::print("======= Backtrace ======\n");
    const auto* end = vm.call_stack_.end();
    for (auto* frame = vm.call_stack_.begin(); frame < end; ++frame) {
      std::string_view word_name;
      if (frame->word != nullptr) {
        word_name = *cast<Word>(frame->word)->name;
      } else {
        word_name = "<Anonymous>"sv;
      }
      fmt::print("{}+{}\n", word_name, cast<intptr_t>(frame->offset));
    }
  }

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
