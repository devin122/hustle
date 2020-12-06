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
