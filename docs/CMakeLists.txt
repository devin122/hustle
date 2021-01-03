################################################################################
# Copyright (c) 2021, Devin Nakamura
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
option(HUSTLE_DOXYGEN "Generate doxygen documentation")
set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doxygen" CACHE PATH "Where to generate doxygen documentation")

if(HUSTLE_DOXYGEN)
    find_package(Doxygen REQUIRED dot)

    set(DOXYGEN_EXCLUDE_PATTERNS
        "*/vcpkg_installed/*"
        "${Hustle_SOURCE_DIR}/extern/*"
    )
    set(DOXYGEN_INCLUDE_PATH
       # ${hustle_SOURCE_DIR}/include
        ${Hustle_BINARY_DIR}
    )
    set(DOXYGEN_STRIP_FROM_PATH
        ${Hustle_SOURCE_DIR}
    )
    set(DOXYGEN_FILE_PATTERNS
        "*.cpp"
        "*.c"
        "*.h"
        "*.hpp"
        "*.def"
    )
    set(DOXYGEN_EXTENSION_MAPPING "def=C++")
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_MACRO_EXPANSION YES)
    set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
    set(DOXYGEN_EXPAND_AS_DEFINED
        MAKE_CMD
    )
    doxygen_add_docs(
        doxygen
        ${Hustle_SOURCE_DIR}
        COMMENT "Generate Doxygen documentation"
    )
endif()
