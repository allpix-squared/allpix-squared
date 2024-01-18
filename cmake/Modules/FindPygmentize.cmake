# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# CMake script to find the pygmentize executable and export it as Pygmentize::pygmentize target

FIND_PROGRAM(
    PYGMENTIZE_EXECUTABLE
    NAMES pygmentize
)
MARK_AS_ADVANCED(PYGMENTIZE_EXECUTABLE)

IF(PYGMENTIZE_EXECUTABLE)
    EXECUTE_PROCESS(
        COMMAND "${PYGMENTIZE_EXECUTABLE}" "-V"
        OUTPUT_VARIABLE _Pygmentize_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _Pygmentize_version_result
        ERROR_QUIET)

    IF(_Pygmentize_version_result)
        MESSAGE(WARNING "Unable to determine pygmentize version: ${_Pygmentize_version_result}")
    ELSE()
        STRING(REGEX REPLACE "^.+ version ([^\n,]+),.*" "\\1" PYGMENTIZE_VERSION "${_Pygmentize_version}")
    ENDIF()

    IF(NOT TARGET Pygmentize::pygmentize)
        ADD_EXECUTABLE(Pygmentize::pygmentize IMPORTED GLOBAL)
        SET_TARGET_PROPERTIES(Pygmentize::pygmentize PROPERTIES
            IMPORTED_LOCATION "${PYGMENTIZE_EXECUTABLE}")
    ENDIF()
ENDIF()

IF(TARGET Pygmentize::pygmentize)
    SET(PYGMENTIZE_FOUND TRUE)
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    Pygmentize
    REQUIRED_VARS PYGMENTIZE_EXECUTABLE
    VERSION_VAR PYGMENTIZE_VERSION)
