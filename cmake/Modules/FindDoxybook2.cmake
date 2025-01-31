# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# CMake script to find the doxybook2 executable and export it as Doxybook2::doxybook2 target

FIND_PROGRAM(
    DOXYBOOK2_EXECUTABLE
    NAMES doxybook2
)
MARK_AS_ADVANCED(DOXYBOOK2_EXECUTABLE)

IF(DOXYBOOK2_EXECUTABLE)
    EXECUTE_PROCESS(
        COMMAND "${DOXYBOOK2_EXECUTABLE}" "--version"
        OUTPUT_VARIABLE _Doxybook2_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _Doxybook2_version_result
        ERROR_QUIET)

    IF(_Doxybook2_version_result)
        MESSAGE(WARNING "Unable to determine doxybook2 version: ${_Doxybook2_version_result}")
    ELSE()
        # FIXME
        STRING(REGEX REPLACE "^v(.*)" "\\1" DOXYBOOK2_VERSION "${_Doxybook2_version}")
    ENDIF()

    IF(NOT TARGET Doxybook2::doxybook2)
        ADD_EXECUTABLE(Doxybook2::doxybook2 IMPORTED GLOBAL)
        SET_TARGET_PROPERTIES(Doxybook2::doxybook2 PROPERTIES
            IMPORTED_LOCATION "${DOXYBOOK2_EXECUTABLE}")
    ENDIF()
ENDIF()

IF(TARGET Doxybook2::doxybook2)
    SET(DOXYBOOK2_FOUND TRUE)
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    Doxybook2
    REQUIRED_VARS DOXYBOOK2_EXECUTABLE
    VERSION_VAR DOXYBOOK2_VERSION)
