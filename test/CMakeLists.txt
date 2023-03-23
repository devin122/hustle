################################################################################
# Copyright (c) 2020, Devin Nakamura
#
# SPDX-License-Identifier: BSD-2-Clause
################################################################################

find_package(Catch2 CONFIG REQUIRED)

set(CMAKE_FOLDER test)

add_library(test-main OBJECT main.cpp)
add_dependencies(test-main hustle-generated)
target_link_libraries(test-main PUBLIC Catch2::Catch2)

add_subdirectory(gc)
add_subdirectory(vm)
add_subdirectory(serialize)
add_subdirectory(unittest)
add_subdirectory(support)
