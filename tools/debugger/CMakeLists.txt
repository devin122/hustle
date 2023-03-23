################################################################################
# Copyright (c) 2020, Devin Nakamura
#
# SPDX-License-Identifier: BSD-2-Clause
################################################################################

hustle_add_executable(hdb 
    main.cpp
    CommandManager.cpp
    Commands.cpp
)

target_link_libraries(hdb
    HustleVM
    HustleSupport
    HustleGC
    fmt::fmt
    CLI11::CLI11
    HustleParser
    REPLXX::replxx
    std::filesystem
)
