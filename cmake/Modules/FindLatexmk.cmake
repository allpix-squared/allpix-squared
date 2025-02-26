# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# CMake script to find the latexmk executable and export it as Latexmk::latexmk target

FIND_PROGRAM(
    LATEXMK_EXECUTABLE
    NAMES latexmk
)
MARK_AS_ADVANCED(LATEXMK_EXECUTABLE)

IF(LATEXMK_EXECUTABLE)
    EXECUTE_PROCESS(
        COMMAND "${LATEXMK_EXECUTABLE}" "-v"
        OUTPUT_VARIABLE _Latexmk_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _Latexmk_version_result
        ERROR_QUIET)

    IF(_Latexmk_version_result)
        MESSAGE(WARNING "Unable to determine latexmk version: ${_Latexmk_version_result}")
    ELSE()
        STRING(REGEX REPLACE "^.+Version ([^\n]+)" "\\1" LATEXMK_VERSION "${_Latexmk_version}")
    ENDIF()

    IF(NOT TARGET Latexmk::latexmk)
        ADD_EXECUTABLE(Latexmk::latexmk IMPORTED GLOBAL)
        SET_TARGET_PROPERTIES(Latexmk::latexmk PROPERTIES
            IMPORTED_LOCATION "${LATEXMK_EXECUTABLE}")
    ENDIF()
ENDIF()

IF(TARGET Latexmk::latexmk)
    SET(LATEXMK_FOUND TRUE)
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    Latexmk
    REQUIRED_VARS LATEXMK_EXECUTABLE
    VERSION_VAR LATEXMK_VERSION)
