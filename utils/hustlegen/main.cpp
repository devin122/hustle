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
    generate_file(output_file, write_class_defs, input_file);
  });

  app.add_subcommand("tags", "Generate class tags")->callback([&] {
    generate_file(output_file, write_tag_file, input_file);
  });

  CLI11_PARSE(app, argc, argv);
  return 0;
}
