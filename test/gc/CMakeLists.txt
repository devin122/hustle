################################################################################
# Copyright (c) 2020, Devin Nakamura
#
# SPDX-License-Identifier: BSD-2-Clause
################################################################################

hustle_add_executable(hustle-gc-test
    GCTest.cpp
)

target_link_libraries(hustle-gc-test
    PRIVATE
    Catch2::Catch2WithMain
    HustleVM
    HustleGC
)
add_test(NAME gc-test COMMAND hustle-gc-test -r junit -o gc_test.xml)
