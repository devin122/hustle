################################################################################
# Copyright (c) 2020, Devin Nakamura
#
# SPDX-License-Identifier: BSD-2-Clause
################################################################################

hustle_add_library(HustleGC STATIC
    ${CMAKE_SOURCE_DIR}/include/hustle/GC.hpp
    gc.cpp
)

target_link_libraries(HustleGC PUBLIC Microsoft.GSL::GSL)

add_dependencies(HustleGC hustle-generated)

install(
    TARGETS HustleGC
    ARCHIVE
    COMPONENT development
)
