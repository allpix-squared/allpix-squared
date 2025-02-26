# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# CMake script to find the pandoc executable and export it as Pandoc::pandoc target

FIND_PROGRAM(
    PANDOC_EXECUTABLE
    NAMES pandoc
)
MARK_AS_ADVANCED(PANDOC_EXECUTABLE)

IF(PANDOC_EXECUTABLE)
    EXECUTE_PROCESS(
        COMMAND "${PANDOC_EXECUTABLE}" "-v"
        OUTPUT_VARIABLE _Pandoc_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _Pandoc_version_result
        ERROR_QUIET)

    IF(_Pandoc_version_result)
        MESSAGE(WARNING "Unable to determine pandoc version: ${_Pandoc_version_result}")
    ELSE()
        STRING(REGEX REPLACE "^pandoc ([^\n]+)\n.*" "\\1" PANDOC_VERSION "${_Pandoc_version}")
    ENDIF()

    IF(NOT TARGET Pandoc::pandoc)
        ADD_EXECUTABLE(Pandoc::pandoc IMPORTED GLOBAL)
        SET_TARGET_PROPERTIES(Pandoc::pandoc PROPERTIES
            IMPORTED_LOCATION "${PANDOC_EXECUTABLE}")
    ENDIF()
ENDIF()

IF(TARGET Pandoc::pandoc)
    SET(PANDOC_FOUND TRUE)
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    Pandoc
    REQUIRED_VARS PANDOC_EXECUTABLE
    VERSION_VAR PANDOC_VERSION)
