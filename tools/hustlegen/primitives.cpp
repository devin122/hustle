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

#include "hustlegen.hpp"

#include <string>
using std::string;
void write_primitives(IndentingStream& out, ParseType data) {
  auto it = data.begin();
  auto entries = kv_node(data);

  // Forward declarations of functions
  for (const auto [prim_name, func_name] : entries) {
    out.writeln("static void {}(VM *vm, Quotation*);", func_name.as<string>());
  }
  out.nl();

  // Write the primitive tables
  out.writeln("static constexpr std::pair<const char *, VM::CallType> "
              "primitives[] = {{")
      .indent();
  for (auto [prim_name, func_name] : entries) {
    out.writeln("{{\"{}\", &{}}},", prim_name.as<string>(),
                func_name.as<string>());
  }
  out.outdent().writeln("}};");
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
