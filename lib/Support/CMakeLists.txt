################################################################################
# Copyright (c) 2020, Devin Nakamura
#
# SPDX-License-Identifier: BSD-2-Clause
################################################################################

hustle_add_library(HustleSupport STATIC
    assert.cpp
    utility.cpp
)

if(WIN32)
    target_sources(HustleSupport PRIVATE windows/Memory.cpp)
else()
    target_sources(HustleSupport PRIVATE unix/Memory.cpp)
endif()


# TODO  check if we are doing an IDE build
file(GLOB headers ${Hustle_SOURCE_DIR}/include/hustle/Support/*.hpp)
target_sources(HustleSupport PRIVATE ${headers})

install(
    TARGETS HustleSupport
    ARCHIVE
    COMPONENT development
)
