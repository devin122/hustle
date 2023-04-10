################################################################################
# Copyright (c) 2020, Devin Nakamura
#
# SPDX-License-Identifier: BSD-2-Clause
################################################################################

find_package(Catch2 CONFIG REQUIRED)

set(CMAKE_FOLDER test)

add_subdirectory(gc)
add_subdirectory(vm)
add_subdirectory(serialize)
add_subdirectory(unittest)
add_subdirectory(support)
