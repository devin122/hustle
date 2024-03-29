/*
 * Copyright (c) 2020, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef HUSTLE_BINARY_STREAM_HPP
#define HUSTLE_BINARY_STREAM_HPP

#include <istream>
#include <ostream>
#include <stdint.h>
#include <type_traits>
namespace hustle {

#if 0
// TODO replace this with std endian when we can
enum class Endian {
  Little,
  Big,
};

class BinaryStream {
public:
};

template <Endian endian>
class BinaryReader {
public:
  BinaryReader& operator>>(uint8_t& v) {
    v = read_byte();
    return *this;
  }
  template <typename T>
  std::enable_if_t<std::is_integral_v<T>, BinaryReader&> operator>>(T& v) {
    std::make_unsigned_t<T> value = 0;
    constexpr unsigned bytes = sizeof(T);
    // TODO gcc cant figure out this a a byte swap
    for (unsigned i = 0; i < sizeof(T); ++i) {
      value |= read_byte() << (8 * ((endian == Endian::Little)
                                        ? i
                                        : (sizeof(T) - i - 1)));
    }
    return *this
  }

  uint8_t read_byte();
};

class BinaryWriter {};
#endif

class OStreamAdapter {};
class IStreamAdapter {};

template <typename T>
constexpr bool is_primitive = std::is_integral_v<T>;
// TODO right now we don't support endian conversion
// template<typename Adapter>
class BinaryReader {
public:
  BinaryReader(std::istream& is) : is_(is) {}

  // operator>> for primitive types
  template <typename T, typename std::enable_if_t<is_primitive<T>>* = nullptr>
  BinaryReader& operator>>(T& v) {
    // read_bytes(std::addressof(v), )
    read_bytes(&v, sizeof(T));
    return *this;
  }

  // for non primitive types use the serialize method
  template <typename T, typename std::enable_if_t<!is_primitive<T>>* = nullptr>
  BinaryReader& operator>>(T& v) {
    serialize(*this, v);
    return *this;
  }

  BinaryReader& operator>>(std::string& str) {
    size_t size;
    *this >> size;
    str.resize(size);
    // TODO this is gross, but seems like the only safe way of doing this
    for (size_t i = 0; i < size; ++i) {
      *this >> str[i];
    }
    return *this;
  }
  template <typename T>
  BinaryReader& operator|(T& v) {
    *this >> v;
    return *this;
  }
  void read_bytes(void* ptr, size_t sz) { is_.read((char*)ptr, sz); }

private:
  std::istream& is_;
};

// template<typename Adapter>
class BinaryWriter {
public:
  BinaryWriter(std::ostream& os) : os_(os) {}

  template <typename T, typename std::enable_if_t<is_primitive<T>>* = nullptr>
  BinaryWriter& operator<<(T v) {
    write_bytes(&v, sizeof(T));
    return *this;
  }

  template <typename T, typename std::enable_if_t<!is_primitive<T>>* = nullptr>
  BinaryWriter& operator<<(T& v) {
    serialize(*this, v);
    return *this;
  }

  BinaryWriter& operator<<(std::string& str) {
    size_t str_size = str.size();
    *this << str_size;
    write_bytes(str.data(), str_size);
    return *this;
  }

  template <typename T>
  BinaryWriter& operator|(T& v) {
    *this << v;
    return *this;
  }

  void write_bytes(const void* ptr, size_t size) {
    os_.write((const char*)ptr, size);
  }

private:
  std::ostream& os_;
};

} // namespace hustle

#endif
