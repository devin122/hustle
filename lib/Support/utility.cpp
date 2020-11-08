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

#include <cassert>
#include <filesystem>
#include <hustle/Support/Utility.hpp>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace {
const char* saved_argv0 = nullptr;
std::filesystem::path init_cwd;
std::filesystem::path self;
} // namespace

void hustle::save_argv0(const char* argv0) {
  saved_argv0 = argv0;
  init_cwd = std::filesystem::current_path();
  self = std::filesystem::canonical(argv0);
}

std::filesystem::path hustle::hustle_lib_dir() {
  auto exe_path = get_exe_path();

  return exe_path.parent_path().parent_path().append("hustle");
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
  constexpr DWORD BUFFER_SIZE = 1024;
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
