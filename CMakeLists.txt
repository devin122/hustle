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

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

set(HUSTLE_VERSION_SUFFIX "-git")
project(hustle
    LANGUAGES CXX
    VERSION 0.1.0)

find_package(Microsoft.GSL CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
#find_package(ryml CONFIG REQUIRED)
find_package(replxx CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(cityhash CONFIG REQUIRED)



set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

find_package(Filesystem REQUIRED)
include(GNUInstallDirs)
include(AddHustle)
include(CPack)


#setup out output paths
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${HUSTLE_BUILD_CFG_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${HUSTLE_BUILD_CFG_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${HUSTLE_BUILD_CFG_DIR}/lib")


if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GCC")
    set(CMAKE_CXX_FLAGS "-fcolor-diagnostics ${CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "-fcolor-diagnostics ${CMAKE_C_FLAGS}")
endif()
function(add_hustlegen func input output)
    add_custom_command(
        OUTPUT ${output}
        DEPENDS
            ${input}
            $<TARGET_FILE:hustlegen>
        COMMAND hustlegen ${func} -o "${output}" ${input}
    )
endfunction()

enable_testing()

add_subdirectory(extern)

# now that we have finished defining any external apps, we can safely pollute global options
include_directories(include ${HUSTLE_BUILD_CFG_DIR} ${CMAKE_CURRENT_BINARY_DIR}/include ${CMAKE_CURRENT_BINARY_DIR})

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w14996")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # TODO: this is a bodge to work arround broken code
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-constexpr")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)
add_subdirectory(utils)

if(HUSTLE_CODE_COVERAGE)
    set(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping ${CMAKE_CXX_FLAGS}")
endif()

if(HUSTLE_ASAN)
    set(CMAKE_CXX_FLAGS "-fsanitize=address ${CMAKE_CXX_FLAGS}")
endif()
if(HUSTLE_UBSAN)
    set(CMAKE_CXX_FLAGS "-fsanitize=undefined ${CMAKE_CXX_FLAGS}")
endif()

add_hustlegen("tags" "${CMAKE_CURRENT_SOURCE_DIR}/classes.json" "${CMAKE_CURRENT_BINARY_DIR}/cell_tags.def")
add_hustlegen("classes" "${CMAKE_CURRENT_SOURCE_DIR}/classes.json" "${CMAKE_CURRENT_BINARY_DIR}/classes.def")
add_hustlegen("primitives" "${CMAKE_CURRENT_SOURCE_DIR}/primitives.yaml" "${CMAKE_CURRENT_BINARY_DIR}/primitives.def")


add_custom_target(hustle-generated
    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/cell_tags.def
        ${CMAKE_CURRENT_BINARY_DIR}/classes.def
        ${CMAKE_CURRENT_BINARY_DIR}/primitives.def
)


add_subdirectory(lib)
add_subdirectory(hustle)
add_subdirectory(tools)

hustle_add_executable(hustle
    main.cpp
    ${BACKWARD_ENABLE}
)
add_backward(hustle)
add_dependencies(hustle hustle-generated)
file(GLOB_RECURSE global_headers "${CMAKE_SOURCE_DIR}/include/hustle/*.h" "${CMAKE_SOURCE_DIR}/include/hustle/*.hpp")
target_sources(hustle PRIVATE ${global_headers} ${CMAKE_CURRENT_SOURCE_DIR}/utils/hustle.natvis)
target_link_libraries(hustle
    HustleVM
    HustleSupport
    HustleGC
    fmt::fmt
    CLI11::CLI11
    HustleParser
    REPLXX::replxx
    std::filesystem
)
install(
    TARGETS hustle
    RUNTIME DESTINATION ${CMAKE_INSTALL_DIR_BINDIR}
)

add_subdirectory(test)

# configure_file does not like generator expressions so we cant put output under HUSTLE_BUILD_CFG_DIR.
# However, the generated header shouldn't be different based on build config.
configure_file(include/hustle/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/hustle/config.h)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/hustle
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT development
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
)
