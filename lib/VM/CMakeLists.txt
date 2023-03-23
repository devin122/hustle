################################################################################
# Copyright (c) 2020, Devin Nakamura
#
# SPDX-License-Identifier: BSD-2-Clause
################################################################################


hustle_add_library(HustleVM STATIC
    Array.cpp
    primitives.cpp
    StackDump.cpp
    Stack.cpp
    VM.cpp
    debug.cpp
)
target_link_libraries(HustleVM
    PRIVATE
        Microsoft.GSL::GSL
        fmt::fmt
        cityhash
        HustleParser
        HustleSupport
    PUBLIC
        std::filesystem
    )
target_include_directories(HustleVM PRIVATE ${CMAKE_BINARY_DIR})
add_dependencies(HustleVM hustle-generated)

install(
    TARGETS HustleVM
    ARCHIVE
    COMPONENT development
)
