/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "BinaryStream.hpp"
#include <catch2/catch.hpp>
#include <fmt/core.h>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <stdlib.h>
#include <string>

using namespace hustle;

TEST_CASE("BinaryReader", "[BinaryReader]") {
  int i = 42;
  std::string str((char*)&i, sizeof(i));

  std::istringstream os(str);

  BinaryReader rd(os);
  int j = 0;
  rd >> j;
  CHECK(j == i);
}

TEST_CASE("BinaryWriter") {
  std::ostringstream ss;
  BinaryWriter bw(ss);
  uint32_t val = 453;
  bw << val;
  auto string = ss.str();
  CHECK(memcmp(string.c_str(), &val, sizeof(val)) == 0);
}

TEST_CASE("Round trip conversion") {
  int out_1 = 3;
  short out_2 = 21;
  uint64_t out_3 = 48921645563;
  int out_4 = -432;

  std::stringstream ss;

  BinaryWriter bw(ss);

  bw << out_1 << out_2 << out_3 << out_4;

  int in_1 = 0;
  short in_2 = 0;
  uint64_t in_3 = 0;
  int in_4 = 0;

  BinaryReader br(ss);

  br >> in_1 >> in_2 >> in_3 >> in_4;

  CHECK(in_1 == out_1);
  CHECK(in_2 == out_2);
  CHECK(in_3 == out_3);
  CHECK(in_4 == out_4);
}

/*
namespace {
struct Foo {
  int a = 0;
  char b = 0;
  uint16_t c = 0;

  constexpr bool operator==(const Foo& other) const noexcept {
    return (a == other.a) && (b == other.b) && (c == other.c);
  }
};
} // namespace

template <typename T>
void serialize(T& stream, Foo& x) {
  stream | x.a | x.b | x.c;
}

TEST_CASE("Struct serialization") {
  Foo output{-20775, 'Z', 665};
  std::stringstream ss;

  BinaryWriter bw(ss);
  bw << output;

  Foo input;
  BinaryReader br(ss);
  br >> input;
  CHECK(input == output);
}

TEST_CASE("Serialize strings") {
  std::string out_string = "Hello world!";

  std::stringstream ss;
  BinaryWriter bw(ss);
  bw << out_string;

  BinaryReader br(ss);
  std::string in_string;
  br >> in_string;

  CHECK(out_string == in_string);
}
*/
