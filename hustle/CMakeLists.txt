################################################################################
# Copyright (c) 2020, Devin Nakamura
#
# SPDX-License-Identifier: BSD-2-Clause
################################################################################

set(stdlib_files
    kernel.hsl
)


set(out_files)
foreach(f ${stdlib_files})
    set(src ${CMAKE_CURRENT_SOURCE_DIR}/${f})
    set(dst ${Hustle_BINARY_DIR}/hustle/${f})
    set(dst "lib/hustle/${f}")
    set(output "${Hustle_BINARY_DIR}/${dst}")
    add_custom_command(
        OUTPUT "${output}"
        DEPENDS ${src}
        COMMAND ${CMAKE_COMMAND} -E copy ${src} "${HUSTLE_BUILD_CFG_DIR}/${dst}"
        COMMENT "Copying Hustlelib: ${f}"
    )
    list(APPEND out_files "${output}")

    # for multi-config generators, the output name is symbolic
    if(HUSTLE_MULTI_CONFIG)
        set_source_files_properties("${output}" PROPERTIES SYMBOLIC TRUE)
    endif()
endforeach()

add_custom_target(hustle-lib ALL
DEPENDS ${out_files}
)

install(
    FILES ${stdlib_files}
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/hustle"
)
