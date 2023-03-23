################################################################################
# Copyright (c) 2020, Devin Nakamura
#
# SPDX-License-Identifier: BSD-2-Clause
################################################################################

hustle_add_library(HustleParser STATIC
    BootstrapLexer.cpp
    Lexer.cpp
    InteractiveStreamBuff.cpp
)

# TODO  check if we are doing an IDE build
file(GLOB headers ${Hustle_SOURCE_DIR}/include/hustle/Parser/*.hpp)
target_sources(HustleParser PRIVATE ${headers})
target_link_libraries(HustleParser Microsoft.GSL::GSL)

install(
    TARGETS HustleParser
    ARCHIVE
    COMPONENT development
)
