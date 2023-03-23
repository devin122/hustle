################################################################################
# Copyright (c) 2020, Devin Nakamura
#
# SPDX-License-Identifier: BSD-2-Clause
################################################################################

hustle_add_executable(hustle-test
    main.cpp
)
#
#target_link_libraries(hustle-vm-test test-main HustleVM HustleGC)
#add_test(NAME vm-test COMMAND hustle-vm-test -r junit -o vm_test.xml)

target_link_libraries(hustle-test
    PRIVATE
    CLI11::CLI11
    HustleVM
    HustleGC
    HustleSupport
    std::filesystem
    fmt::fmt
)

function(hustle_unit_test name source)
    add_test(
        NAME "unit-${name}"
        COMMAND
            hustle-test
            ${source}
            --name=${name}
            --xml=${name}_test.xml
    )
endfunction()


hustle_unit_test(primitives ${CMAKE_CURRENT_SOURCE_DIR}/primitives.hsl)
hustle_unit_test(trailing-nl ${CMAKE_CURRENT_SOURCE_DIR}/trailing-nl.hsl)
