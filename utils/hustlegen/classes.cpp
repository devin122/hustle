/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "hustlegen.hpp"
#include <list>
#include <string>
using namespace std::literals;

using std::string;

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

template <typename F>
static void with_classes(F func, IndentingStream& stream,
                         const std::string& input_file) {
  auto yaml = YAML::LoadFile(input_file);
  std::list<Class> classes;
  auto classes_node = kv_node(yaml["classes"]);
  for (auto [key, val] : classes_node) {
    Class c;
    c.name = key.template as<std::string>();
    parse_class(c, val);
    classes.emplace_back(std::move(c));
  }
  func(stream, classes);
}

/* #region  Tag_file */
/// Generate the cell_tag enum
static void output_class_tags(IndentingStream& out, const ClassList& classes) {
  out.writeln("enum cell_tag {{");
  out.indent();
  out.writeln("CELL_INT = 0,");
  for (auto& cl : classes) {
    std::string enum_name = cl.name;
    out.writeln("{},", cl.enum_tag_name);
  }
  out.writeln("CELL_TAG_MAX");
  out.outdent();
  out.writeln("}};");
}

/// Generate the get_type_name() funcion
static void output_tag_to_string(IndentingStream& out,
                                 const ClassList& classes) {
  out.writeln("inline const char *get_type_name(cell_t c){{");
  out.indent();

  out.writeln("switch(c){{");
  out.indent();
  out.writeln("case CELL_INT: return \"int\";");

  for (auto& cl : classes) {
    out.writeln("case {}: return \"{}\";", cl.enum_tag_name, cl.name);
  }

  out.writeln("default: return \"unknown\";").outdent();
  out.writeln("}}").outdent();
  out.writeln("}}");
}

static void write_tag_file_impl(IndentingStream& out, ClassList& classes) {
  output_class_tags(out, classes);
  out.nl();
  output_tag_to_string(out, classes);
}

void write_tag_file(IndentingStream& out, const std::string& input_file) {
  with_classes(write_tag_file_impl, out, input_file);
}

/* #endregion */

static void output_class_decls(IndentingStream& out, const ClassList& classes) {
  for (auto& cl : classes) {
    out.writeln("struct {};", cl.name);
  }
  out.writeln("");
}

/**
 * Write out the definitions for the given set of classes.
 *
 * \note no definition is written if a class has no members, as
 * we take that as a sign that the user is handling the defintion.
 */
static void print_class_defs(IndentingStream& out, const ClassList& classes) {
  for (auto& cl : classes) {
    if (cl.members.empty()) {
      // If the class has no members, assume that that someone else is handling
      // the definition.
      continue;
    }
    out.writeln("struct {} : public Object {{", cl.name).indent();
    out.writeln("static constexpr cell_tag TAG_VALUE = {};", cl.enum_tag_name);
    out.writeln("~{}() = delete;", cl.name);

    for (auto& member : cl.members) {
      out.writeln("{}", member);
    }

    out.outdent();
    out.writeln("}} HUSTLE_HEAP_ALLOCATED;\n");
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

static void write_class_defs_impl(IndentingStream& out,
                                  const ClassList& classes) {
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

void write_class_defs(IndentingStream& out, const std::string& input_file) {
  with_classes(write_class_defs_impl, out, input_file);
}
