/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "hustlegen.hpp"
#include <string>
using std::string;

static void write_forward_decls(IndentingStream& out, const YAML::Node& data) {
  out.writeln("// Forward declarations").nl();
  out.writeln("// Primitive words");
  for (const auto [prim_name, func_name] : kv_node(data["words"])) {
    out.writeln("static void {}(VM *vm, Quotation*);", func_name.as<string>());
  }

  out.nl().writeln("// Parse words");
  for (const auto [prim_name, func_name] : kv_node(data["parse_words"])) {
    out.writeln("static void {}(VM *vm, Quotation*);", func_name.as<string>());
  }
  out.nl();
}

static void write_function_table(IndentingStream& out, const YAML::Node& data,
                                 const string& name) {
  out.writeln("static constexpr std::pair<const char *, VM::CallType> "
              "{}[] = {{",
              name);
  out.indent();
  for (auto [prim_name, func_name] : kv_node(data)) {
    out.writeln("{{\"{}\", &{}}},", prim_name.as<string>(),
                func_name.as<string>());
  }
  out.outdent().writeln("}};").nl();
}

void write_primitives(IndentingStream& out, ParseType data) {

  write_forward_decls(out, data);
  auto it = data.begin();
  auto entries = kv_node(data);

  write_function_table(out, data["words"], "primitives");
  write_function_table(out, data["parse_words"], "parse_primitives");
}

void write_primitive_test_cases(IndentingStream& out, ParseType data) {
  out.writeln("// Names of all the primitives we generate");
  auto entries = kv_node(data);
  out.writeln("{{").indent();
  for (auto [prim_name, func_name] : entries) {
    out.writeln("\"{}\"s,", prim_name.as<string>());
  }
  out.outdent().writeln("}};");
}
