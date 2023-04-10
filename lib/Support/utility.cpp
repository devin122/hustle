/*
 * Copyright (c) 2021, Devin Nakamura
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <hustle/Support/Utility.hpp>

#include <cassert>
#include <filesystem>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace {
const char* saved_argv0 = nullptr;
std::filesystem::path init_cwd;
// Removed for now as currently unused.
// std::filesystem::path self;
} // namespace

void hustle::save_argv0(const char* argv0) {
  saved_argv0 = argv0;
  init_cwd = std::filesystem::current_path();
  // This is broken when exe is invoked via $PATH.
  // Since it is currently not used, just disable it for now
  // self = std::filesystem::canonical(argv0);
}

std::filesystem::path hustle::hustle_lib_dir() {
  auto exe_path = get_exe_path();

  return exe_path.parent_path().parent_path().append("lib/hustle");
}

std::filesystem::path hustle::get_exe_path() {
#if defined(_WIN32)
  constexpr DWORD BUFFER_SIZE = 1024;
  char buffer[BUFFER_SIZE];
  auto rc = GetModuleFileNameA(NULL, buffer, BUFFER_SIZE);
  assert(rc < BUFFER_SIZE);
  assert(rc != 0);
  return std::filesystem::canonical(buffer);
#elif defined(__linux__)
  return std::filesystem::canonical("/proc/self/exe");
#elif defined(__APPLE__)
  constexpr size_t BUFFER_SIZE = 1024;
  uint32_t buffsize = BUFFER_SIZE;
  char buffer[BUFFER_SIZE];
  auto rc = _NSGetExecutablePath(buffer, &buffsize);
  assert(rc == 0);
  return std::filesystem::canonical(buffer);
#else
  // Other operating systems currently unsupported.
  assert(false);
#endif
}
