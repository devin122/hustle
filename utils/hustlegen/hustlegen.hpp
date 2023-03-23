/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef HUSTLEGEN_HPP
#define HUSTLEGEN_HPP

#include <hustle/Support/IndentingStream.hpp>
#include <utility>
#include <yaml-cpp/yaml.h>

struct Class {
  std::string name;
  std::string enum_tag_name;
  std::list<std::string> members; // TODO: this is pretty hackey
};
using ClassList = std::list<Class>;
using hustle::IndentingStream;

// Alias for changing parser
using ParseType = YAML::Node;

void write_primitives(hustle::IndentingStream& out, ParseType data);
void write_primitive_test_cases(hustle::IndentingStream& out, ParseType data);
void write_tag_file(hustle::IndentingStream& out,
                    const std::string& input_file);
void write_class_defs(hustle::IndentingStream& out,
                      const std::string& input_file);

struct KVWrapper {
  struct Iterator : public YAML::Node::iterator {
    Iterator(const YAML::Node::iterator& it) : YAML::Node::iterator(it) {}

    std::pair<YAML::Node, YAML::Node> operator*() {
      return YAML::Node::iterator::operator*();
    }
  };

  // struct ConstIterator: public YAML::Node::it
  KVWrapper(YAML::Node n) : wrapped_(std::move(n)) {}

  Iterator begin() { return {wrapped_.begin()}; }
  Iterator end() { return {wrapped_.end()}; }

private:
  YAML::Node wrapped_;
};

static inline KVWrapper kv_node(YAML::Node node) {
  return KVWrapper(std::move(node));
}

#endif
