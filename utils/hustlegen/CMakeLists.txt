################################################################################
# Copyright (c) 2020, Devin Nakamura
#
# SPDX-License-Identifier: BSD-2-Clause
################################################################################

hustle_add_executable(hustlegen
    classes.cpp
    primitives.cpp
    main.cpp
    ${BACKWARD_ENABLE}

    NO_GENERATED_DEPENDS
)

add_backward(hustlegen)


target_include_directories(hustlegen PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(hustlegen
    PRIVATE
        CLI11::CLI11
        fmt::fmt
        nlohmann_json::nlohmann_json
        yaml-cpp
        std::filesystem
)

install(
    TARGETS hustlegen
    RUNTIME DESTINATION ${CMAKE_INSTALL_DIR_BINDIR}
    COMPONENT tools
)
