/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include "hustle/Parser/BootstrapLexer.hpp"
#include "hustle/Object.hpp"
#include "hustle/Parser/Lexer.hpp"
#include "hustle/VM.hpp"
#include <cctype>
#include <iostream>
using namespace hustle::bootstrap;
using namespace hustle;

std::vector<Cell> hustle::bootstrap::tokenize(Iterator& it, const Iterator& end,
                                              VM& vm, char until) {

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
      result.push_back(Cell::from_raw(cell_helpers::make_cell(vm_str)));
      it = str_end + 1;
    } else if (*it == '#') {
      // Line comment, we are done
      break;
    } else {
      auto token_end = std::find_if(it, end, (int (*)(int))std::isspace);
      if ((token_end - it) == 1) {
        if (*it == until) {
          it = token_end;
          return result;
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
            result.push_back(quote);
          } else {
            result.push_back(arr);
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
          result.push_back(Cell::from_raw(cell_helpers::make_cell(ll)));
          continue;
        }
      }
      if (token_name == "False") {
        result.push_back(vm.globals.False);
      } else if (token_name == "True") {
        result.push_back(vm.globals.True);
      } else {
        try {
          result.push_back(Cell::from_raw(vm.lookup_symbol(token_name)));
        } catch (std::runtime_error& e) {
          std::cerr << "Unknown symbol " << token_name << std::endl;
          throw;
        }
      }
    }
  }
  if (until != 0) {
    std::cerr << "Error: expected " << until << std::endl;
    throw std::runtime_error("parse error");
  }
  return result;
}
