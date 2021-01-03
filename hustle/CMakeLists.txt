################################################################################
# Copyright (c) 2020, Devin Nakamura
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# SPDX-License-Identifier: BSD-2-Clause
################################################################################

set(stdlib_files
    kernel.hsl
    literals.hsl
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
