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
#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <algorithm>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <hustle/config.h>
#include <iostream>
#include <list>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdio.h>
#include <string>
#include <yaml-cpp/yaml.h>
using json = nlohmann::ordered_json;

using namespace std::literals;
namespace fs = std::filesystem;
using std::string;

static std::string read_file(const char* name) {
  std::stringstream buff;
  std::ifstream input(name);
  buff << input.rdbuf();
  return buff.str();
}

static bool file_eq(const fs::path& path, const std::string& str) {

  std::ifstream input(path);
  std::stringstream buff;
  buff << input.rdbuf();
  return buff.str() == str;
}

static void write_file_content(const fs::path& path, const string& content) {
  auto status = fs::status(path);
  if (fs::exists(status) && file_eq(path, content)) {
    fmt::print("File contents identical\n");
    return;
  }
  std::ofstream fout(path);
  fout << content;
  return;
}

template <typename S, typename F, typename... Args>
static void generate_file(const S& fname, F generator, Args&&... args) {
  std::stringstream strm;
  IndentingStream out(strm);

  generator(out, std::forward<Args>(args)...);
  if (fname == "-"s) {
    std::cout << "BEGIN OUTPUT:\n";
    std::cout << strm.str();
    std::cout << "END OUTPUT \n";
  } else {

    fs::path path(fname);
    write_file_content(path, strm.str());
  }
}

/*
static void parse_class(Class& clazz, json& value) {
  if(value.contains("tag_name")){
    clazz.enum_tag_name = value.at("tag_name");
  } else {
    clazz.enum_tag_name = "CELL_" + clazz.name;
    for (auto& c : clazz.enum_tag_name) {
      c = toupper((unsigned char)c);
    }
  }

  if (value.contains("methods")) {
    for (auto& m : value["methods"]) {
      clazz.members.push_back(m);
        }
  }
  if (value.contains("fields")) {
    for (auto& m : value["fields"]) {
      clazz.members.push_back(m);
        }
  }
  return;
}*/

static void parse_class(Class& clazz, const YAML::Node& value) {
  if (value["tag_name"]) {
    clazz.enum_tag_name = value["tag_name"].as<string>();
  } else {
    clazz.enum_tag_name = "CELL_" + clazz.name;
    for (auto& c : clazz.enum_tag_name) {
      c = toupper((unsigned char)c);
    }
  }

  if (value["methods"]) {
    for (auto& m : value["methods"]) {
      clazz.members.push_back(m.as<string>());
    }
  }
  if (value["fields"]) {
    for (auto& m : value["fields"]) {
      clazz.members.push_back(m.as<string>());
    }
  }
  return;
}

template <typename F, typename X>
static void with_classes(F func, IndentingStream& stream, X& yaml) {
  std::list<Class> classes;
  auto classes_node = kv_node(yaml["classes"]);
  for (auto [key, val] : classes_node) {
    // fmt::print("Processing class {}\n", key.as<string>());
    Class c;
    c.name = key.template as<std::string>();
    parse_class(c, val);
    classes.emplace_back(std::move(c));
  }
  func(stream, classes);
}

static void output_class_decls(IndentingStream& out, const ClassList& classes) {
  for (auto& cl : classes) {
    out.writeln("struct {};", cl.name);
  }
  out.writeln("");
}

// TODO take output stream
static void print_class_defs(IndentingStream& out, const ClassList& classes) {
  for (auto& cl : classes) {
    out.writeln("struct {} : public Object {{", cl.name).indent();
    out.writeln("static constexpr cell_tag TAG_VALUE = {};", cl.enum_tag_name);
    out.writeln("~{}() = delete;", cl.name);

    for (auto& member : cl.members) {
      out.writeln("{}", member);
    }

    out.outdent();
    out.writeln("}};\n");
  }
}

// Output the dispatch function
static void output_dispatch(IndentingStream& out, const ClassList& classes) {
  // out << "template"
  out.writeln("template<typename T>");
  out.writeln("auto dispatch(Object *obj, T fn){{").indent();
  out.writeln("switch(obj->tag()){{").indent();
  for (auto& cl : classes) {
    out.writeln("case {}: return fn(({}*)obj);", cl.enum_tag_name, cl.name);
  }
  out.writeln("default: abort(); //TODO better error handling").outdent();
  out.writeln("}}").outdent();
  out.writeln("}}");
}

static void output_cell_dispatch(IndentingStream& out,
                                 const ClassList& classes) {
  out.writeln("template<typename T>");
  out.writeln("auto dispatch_cell(cell_t cell, T fn){{").indent();
  out.writeln("switch(get_cell_type(cell)){{").indent();
  out.writeln("case CELL_INT: return fn(get_cell_int(cell));");
  for (auto& cl : classes) {
    out.writeln("case {}: return fn(({}*)get_cell_pointer(cell));",
                cl.enum_tag_name, cl.name);
  }
  out.writeln("default: abort(); //TODO better error handling").outdent();
  out.writeln("}}").outdent();
  out.writeln("}}");
}
static void generate_output(const ClassList& classes) {

  {
    /*
    std::string output_fname = j["class_def_file"];
    std::ofstream outf(output_fname.c_str());
    if (!outf.is_open()) {
      std::cerr << "failed to open class def file\n";
      abort();
    }
    IndentingStream out(outf);
    */
    IndentingStream out(std::cout);
    output_class_decls(out, classes);
    print_class_defs(out, classes);
    out.nl();

    for (auto& cl : classes) {
      out.writeln("static_assert(std::is_standard_layout_v<TypedCell<{}>>);",
                  cl.name);
      out.writeln("static_assert(sizeof(TypedCell<{}>) == sizeof(cell_t));",
                  cl.name);

      // Only availible in c++20
      // out.writeln("static_assert(std::is_layout_compatible_v<TypedCell<{}>,
      // cell_t>);", cl.name);
      out.nl();
    }
  }

  {
    // Generate primitive info
    /*
    std::string output_fname = j.at("primitive_def_file");
    std::ofstream outf(output_fname.c_str());
    if (!outf.is_open()) {
      std::cerr << "failed to open class def file\n";
      abort();
    }
    */
    // IndentingStream out(outf);
    /*
    IndentingStream out(std::cout);
    for (auto& [prim_name, func_name] : j["primitives"].items()) {
      out.writeln("static void {}(VM *vm, Quotation*);", func_name);
    }
    out.nl();
    out.writeln("static constexpr std::pair<const char *, VM::CallType> "
                "primitives[] = {{")
        .indent();
    for (auto& [prim_name, func_name] : j["primitives"].items()) {
      out.writeln("{{\"{}\", &{}}},", prim_name, func_name);
    }
    out.outdent();
    out.writeln("}};");
    */
  }
}

static void write_class_file(IndentingStream& out, const ClassList& classes) {
  output_class_decls(out, classes);
  out.nl();

  print_class_defs(out, classes);
  for (auto& cl : classes) {
    out.writeln("static_assert(std::is_standard_layout_v<TypedCell<{}>>);",
                cl.name);
    out.writeln("static_assert(sizeof(TypedCell<{}>) == sizeof(cell_t));",
                cl.name);

    // Only availible in c++20
    // out.writeln("static_assert(std::is_layout_compatible_v<TypedCell<{}>,
    // cell_t>);", cl.name);
    out.nl();
  }
  output_dispatch(out, classes);
  out.nl();
  output_cell_dispatch(out, classes);
}

static void write_class_file2(IndentingStream& out, YAML::Node yaml) {
  with_classes(write_class_file, out, yaml);
}

int main(int argc, char** argv) {
  std::string output_file = "-";
  std::string input_file;
  CLI::App app{"hustlegen"};
  app.set_version_flag("-v,--version", HUSTLE_VERSION);

  app.fallthrough();
  app.add_option("-o", output_file, "Output file");
  app.add_option("input_file", input_file, "Input yaml file to parse")
      ->check(CLI::ExistingFile);

  app.add_subcommand("primitives", "Generate primitves.def")->callback([&] {
    auto yaml = YAML::LoadFile(input_file);
    generate_file(output_file, write_primitives, yaml["primitives"]);
  });

  app.add_subcommand("classes", "Generate class defintions")->callback([&] {
    generate_file(output_file, write_class_file2, YAML::LoadFile(input_file));
  });

  app.add_subcommand("tags", "Generate class tags")->callback([&] {
    generate_file(output_file, write_tag_file, input_file);
  });

  CLI11_PARSE(app, argc, argv);
  return 0;
}
