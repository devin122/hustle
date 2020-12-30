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

# set up the staging dir
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(is_multi_config)
    set(HUSTLE_BUILD_CFG_DIR "${hustle_BINARY_DIR}/$<CONFIG>" CACHE INTERNAL "")
    set(HUSTLE_MULTI_CONFIG true CACHE INTERNAL "")
else()
    set(HUSTLE_BUILD_CFG_DIR "${hustle_BINARY_DIR}" CACHE INTERNAL "")
    set(HUSTLE_MULTI_CONFIG false CACHE INTERNAL "")
endif()

#[[
macro(setup_output_paths)
    get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(_is_multi_config)
        foreach(build_mode ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER "${build_mode}" CONFIG_SUFFIX)
            set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_SUFFIX} )
    else()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${hustle_BINARY_DIR}/bin")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${hustle_BINARY_DIR}/lib")
    endif()
endmacro()
]]

function(set_compile_flags tgt)
    if(HUSTLE_ENABLE_WARNINGS)
        if(HUSTLE_GNU_COMPILER)
            target_compile_options(${tgt}
                PRIVATE
                    -Wall
                    -Wextra
                    -Wno-unused-parameter
            )
        endif()
    endif()

    if(HUSTLE_WARNINGS_AS_ERRORS)
        if(HUSTLE_GNU_COMPILER)
            target_compile_options(${tgt}
                PRIVATE
                    -Werror
            )
        endif()
    endif()

    if(HUSTLE_CODE_COVERAGE)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${tgt}
                PRIVATE
                    -fprofile-instr-generate
                    -fcoverage-mapping
            )
            target_link_options(${tgt}
                PRIVATE
                    -fprofile-instr-generate
                    -fcoverage-mapping
            )
        else()
            message(WARNING "Unable to enable code coverage ${CMAKE_CXX_COMPILER_ID}")
        endif()
    endif()

    if(HUSTLE_ASAN)
        target_compile_options(${tgt} PRIVATE -fsanitize=address)
    endif()

    if(HUSTLE_UBSAN)
        target_compile_options(${tgt} PRIVATE -fsanitize=undefined)
    endif()

endfunction()

function(set_output_directory name subdir)
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${HUSTLE_BUILD_CFG_DIR}/${subdir}"
        LIBRARY_OUTPUT_DIRECTORY "${HUSTLE_BUILD_CFG_DIR}/${subdir}"
    )
endfunction(set_output_directory)

function(hustle_add_library name)
    cmake_parse_arguments(OPT "NO_GENERATED_DEPENDS" "" "" ${ARGN})
    add_library(${name} ${OPT_UNPARSED_ARGUMENTS})
    set_output_directory(${name} lib)
    if(NOT OPT_NO_GENERATED_DEPENDS)
        add_dependencies(${name} hustle-generated)
    endif()
    set_compile_flags(${name})
endfunction(hustle_add_library)

function(hustle_add_executable name)
    cmake_parse_arguments(OPT "NO_GENERATED_DEPENDS" "" "" ${ARGN})
    add_executable(${name} ${OPT_UNPARSED_ARGUMENTS})
    set_output_directory(${name} bin)
    if(NOT OPT_NO_GENERATED_DEPENDS)
        add_dependencies(${name} hustle-generated)
    endif()
    set_compile_flags(${name})
endfunction(hustle_add_executable)
