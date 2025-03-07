# SPDX-FileCopyrightText: 2021-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Additional targets to perform cmake-lint/cmake-format

FIND_PROGRAM(CMAKELINT "cmake-lint")
IF(CMAKELINT)
    ADD_CUSTOM_TARGET(
        lint-cmake
        COMMAND ${CMAKELINT} ${CHECK_CMAKE_FILES}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Run linter check on CMake code")
ENDIF()

FIND_PROGRAM(CMAKEFORMAT "cmake-format")
IF(CMAKEFORMAT)
    ADD_CUSTOM_TARGET(
        format-cmake
        COMMAND ${CMAKEFORMAT} -i ${CHECK_CMAKE_FILES}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Run formatting tool on CMake code")
ENDIF()
