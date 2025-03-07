# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# CMake script to find the biber executable and export it as Biber::biber target

FIND_PROGRAM(
    BIBER_EXECUTABLE
    NAMES biber
)
MARK_AS_ADVANCED(BIBER_EXECUTABLE)

IF(BIBER_EXECUTABLE)
    EXECUTE_PROCESS(
        COMMAND "${BIBER_EXECUTABLE}" "-v"
        OUTPUT_VARIABLE _Biber_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _Biber_version_result
        ERROR_QUIET)

    IF(_Biber_version_result)
        MESSAGE(WARNING "Unable to determine biber version: ${_Biber_version_result}")
    ELSE()
        STRING(REGEX REPLACE "^.+ version: ([^\n]+)" "\\1" BIBER_VERSION "${_Biber_version}")
    ENDIF()

    IF(NOT TARGET Biber::biber)
        ADD_EXECUTABLE(Biber::biber IMPORTED GLOBAL)
        SET_TARGET_PROPERTIES(Biber::biber PROPERTIES
            IMPORTED_LOCATION "${BIBER_EXECUTABLE}")
    ENDIF()
ENDIF()

IF(TARGET Biber::biber)
    SET(BIBER_FOUND TRUE)
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    Biber
    REQUIRED_VARS BIBER_EXECUTABLE
    VERSION_VAR BIBER_VERSION)
