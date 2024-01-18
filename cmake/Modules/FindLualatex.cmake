# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# CMake script to find the lualatex executable and export it as Lualatex::lualatex target

FIND_PROGRAM(
    LUALATEX_EXECUTABLE
    NAMES lualatex
)
MARK_AS_ADVANCED(LUALATEX_EXECUTABLE)

IF(LUALATEX_EXECUTABLE)
    EXECUTE_PROCESS(
        COMMAND "${LUALATEX_EXECUTABLE}" "-v"
        OUTPUT_VARIABLE _Lualatex_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _Lualatex_version_result
        ERROR_QUIET)

    IF(_Lualatex_version_result)
        MESSAGE(WARNING "Unable to determine lualatex version: ${_Lualatex_version_result}")
    ELSE()
        STRING(REGEX REPLACE "^.+ Version ([^\n ]+) .+\n.*" "\\1" LUALATEX_VERSION "${_Lualatex_version}")
    ENDIF()

    IF(NOT TARGET Lualatex::lualatex)
        ADD_EXECUTABLE(Lualatex::lualatex IMPORTED GLOBAL)
        SET_TARGET_PROPERTIES(Lualatex::lualatex PROPERTIES
            IMPORTED_LOCATION "${LUALATEX_EXECUTABLE}")
    ENDIF()
ENDIF()

IF(TARGET Lualatex::lualatex)
    SET(LUALATEX_FOUND TRUE)
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    Lualatex
    REQUIRED_VARS LUALATEX_EXECUTABLE
    VERSION_VAR LUALATEX_VERSION)
