################################################################################
# Copyright (c) 2020, Devin Nakamura
#
# SPDX-License-Identifier: BSD-2-Clause
################################################################################

hustle_add_executable(hustle_serialize_test
    BinaryReaderTest.cpp
)

target_link_libraries(hustle_serialize_test Catch2::Catch2WithMain HustleSerialize fmt::fmt)

add_test(NAME serialize-test COMMAND hustle_serialize_test -r junit -o serialize_test.xml)
