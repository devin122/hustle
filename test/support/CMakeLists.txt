################################################################################
# Copyright (c) 2021, Devin Nakamura
#
# SPDX-License-Identifier: BSD-2-Clause
################################################################################

hustle_add_executable(hustle-support-test
    MemoryTest.cpp
)

target_link_libraries(hustle-support-test Catch2::Catch2WithMain HustleSupport)
add_test(NAME support-test COMMAND hustle-support-test -r junit -o support_test.xml)
