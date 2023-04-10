################################################################################
# Copyright (c) 2020, Devin Nakamura
#
# SPDX-License-Identifier: BSD-2-Clause
################################################################################

hustle_add_executable(hustle-vm-test
    CellTest.cpp
    FunctionTest.cpp
    PrimitiveTest.cpp
    StackTest.cpp
)

target_link_libraries(hustle-vm-test Catch2::Catch2WithMain HustleVM HustleGC)
add_test(NAME vm-test COMMAND hustle-vm-test -r junit -o vm_test.xml)
