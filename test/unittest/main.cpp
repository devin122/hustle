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

#include "hustle/Object.hpp"
#include "hustle/Support/Assert.hpp"
#include "hustle/VM.hpp"
#include "hustle/config.h"
#include <hustle/Support/IndentingStream.hpp>
#include <hustle/Support/Utility.hpp>

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

// TODO this is copied from the vm, it should be moved to a library
using namespace hustle;
using namespace std::literals;
using string_span = gsl::string_span<gsl::dynamic_extent>;

void register_primitives(VM& vm);

static std::vector<cell_t> tokenize(std::string& str, VM& vm, int flags = 0) {
  std::istringstream s(str);
  std::vector<cell_t> cells;
  while (!s.eof()) {
    // int c = peek
    // TODO check for string literal

    std::string tok;
    s >> tok;
    if (isdigit(tok[0])) {
      size_t size = 0;
      long long ll = std::stoll(tok, &size, 0);
      if (size == tok.length()) {
        cells.push_back(make_cell(ll));
        continue;
      }
    }
    if (tok == "["s) {
    }
    cell_t word = vm.lookup_symbol(tok);
    if (is_a<Word>(word) && get_cell_pointer(word) == nullptr) {
      std::cerr << "Unkown word " << tok << "\n";
      continue;
    }
    cells.push_back(word);
  }
  return cells;
}

using Iterator = string_span::iterator;
// TODO the until thing is a hack
static std::vector<Cell> tokenize(Iterator& it, const Iterator& end, VM& vm,
                                  char until = 0) {
  // auto it = str.begin();
  // auto end = str.end();
  // auto it = str.begin();
  std::vector<Cell> result;
  // auto x =  std::not_fn(std::isspace);
  while (it != end) {
    // skip ws
    it = std::find_if_not(it, end, (int (*)(int))std::isspace);
    if (it == end) {
      break;
    }

    if (*it == '\'') {
      ++it;

      auto str_end = std::find(it, end, '\'');
      if (str_end == end) {
        std::cerr << "error unterminated string" << std::endl;
        throw std::runtime_error("string parse failure");
      }
      String* vm_str = vm.allocate<String>(str_end - it + 1);
      std::copy(it, str_end, vm_str->data());
      vm_str->data()[str_end - it] = 0;
      vm_str->length_raw = Cell::from_int(str_end - it);
      result.push_back(Cell::from_raw(make_cell(vm_str)));
      it = str_end + 1;
    } else if (*it == '#') {
      // Line comment, we are done
      break;
    } else {
      auto token_end = std::find_if(it, end, (int (*)(int))std::isspace);
      if ((token_end - it) == 1) {
        if (*it == until) {
          it = token_end;
          return std::move(result);
        }
        if (*it == '{' || *it == '[') {
          bool parse_quote = *it == '{';
          ++it;
          auto array_contents = tokenize(it, end, vm, parse_quote ? '}' : ']');
          Array* arr = vm.allocate<Array>(array_contents.size());
          std::copy(array_contents.begin(), array_contents.end(), arr->begin());
          if (parse_quote) {
            Quotation* quote = vm.allocate<Quotation>();
            quote->definition = arr;
            result.push_back(Cell::from_raw(make_cell(quote)));
          } else {
            result.push_back(Cell::from_raw(make_cell(arr)));
          }
          continue;
        }
      }

      // string_span token(it, token_end-1);
      std::string token_name(it, token_end);
      it = token_end;

      if (isdigit(token_name[0])) {
        size_t size = 0;
        long long ll = std::stoll(token_name, &size, 0);
        if (size == token_name.length()) {
          result.push_back(Cell::from_raw(make_cell(ll)));
          continue;
        }
      }
      if (token_name == "False") {
        result.push_back(False);
      } else if (token_name == "True") {
        result.push_back(True);
      } else {
        try {
          result.push_back(Cell::from_raw(vm.lookup_symbol(token_name)));
        } catch (std::runtime_error e) {
          std::cerr << "Unknown symbol " << token_name << std::endl;
          throw e;
        }
      }
    }
  }
  if (until != 0) {
    std::cerr << "Error: expected " << until << std::endl;
    throw std::runtime_error("parse error");
  }
  return std::move(result);
}

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
    auto tokens = tokenize(it, tokenize_span.end(), vm);
    for (auto cell : tokens) {
      vm.evaluate(cell);
    }
  }
}

static int tests_run = 0;
static int tests_passed = 0;

static void check_handler(VM* vm, Quotation*) {
  // TODO should be a handle
  auto expected_stack = cast<Array>(vm->stack_.pop());
  ++tests_run;

  vm->call(vm->pop());

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
  auto* my_stream = vm.lexer_.current_stream();
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
  app.add_option("--name", suite_name, "Name of test suite")->required();

  app.add_option("--xml", xml_out, "File to output xml results");

  CLI11_PARSE(app, argc, argv);

  if (xml_out != "") {
    write_xml(xml_out, suite_name, true);
  }

  VM vm;
  register_primitives(vm);
  load_kernel(vm);

  vm.register_primitive("check", check_handler);

  // std::ifstream fstream(input_file);
  vm.lexer_.add_stream(std::make_unique<std::ifstream>(input_file));
  run_test(vm);

  fmt::print("Passed {} out of {} tests\n", tests_passed, tests_run);
  if (tests_passed != tests_run) {
    fmt::print("FAILED {} tests\n", tests_run - tests_passed);
  }

  write_xml(xml_out, suite_name);

  return 0;
}
