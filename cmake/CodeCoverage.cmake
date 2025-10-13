# Copyright (c) 2012 - 2017, Lars Bilke
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# CHANGES:
#
# 2012-01-31, Lars Bilke
# - Enable Code Coverage
#
# 2013-09-17, Joakim Söderberg
# - Added support for Clang.
# - Some additional usage instructions.
#
# 2016-02-03, Lars Bilke
# - Refactored functions to use named parameters
#
# 2017-06-02, Lars Bilke
# - Merged with modified version from github.com/ufz/ogs
#
# 2018-10-29, Simon Spannagel
# - Removed unused targets
#
# USAGE:
#
# 1. Copy this file into your cmake modules path.
#
# 2. Add the following line to your CMakeLists.txt:
#      include(CodeCoverage)
#
# 3. Append necessary compiler flags:
#      APPEND_COVERAGE_COMPILER_FLAGS()
#
# 4. If you need to exclude additional directories from the report, specify them
#    using the COVERAGE_GCOVR_EXCLUDES variable before calling SETUP_TARGET_FOR_COVERAGE_LCOV.
#    Example:
#      set(COVERAGE_GCOVR_EXCLUDES 'dir1/*' 'dir2/*')
#
# 5. Use the functions described below to create a custom make target which
#    runs your test executable and produces a code coverage report.
#
# 6. Build a Debug build:
#      cmake -DCMAKE_BUILD_TYPE=Debug ..
#      make
#      make my_coverage_target
#

INCLUDE(CMakeParseArguments)

# Check prereqs
FIND_PROGRAM(GCOV_PATH gcov)
FIND_PROGRAM(GCOVR_PATH gcovr PATHS ${CMAKE_SOURCE_DIR}/scripts/test)
FIND_PROGRAM(SIMPLE_PYTHON_EXECUTABLE python)

IF(NOT GCOV_PATH)
    MESSAGE(FATAL_ERROR "gcov not found! Aborting...")
ENDIF() # NOT GCOV_PATH

IF("${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
    IF("${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 3)
        MESSAGE(FATAL_ERROR "Clang version must be 3.0.0 or greater! Aborting...")
    ENDIF()
ELSEIF(NOT CMAKE_COMPILER_IS_GNUCXX)
    MESSAGE(FATAL_ERROR "Compiler is not GNU gcc! Aborting...")
ENDIF()

SET(COVERAGE_COMPILER_FLAGS
    "-g -O0 --coverage -fprofile-arcs -fprofile-update=atomic -ftest-coverage"
    CACHE INTERNAL "")

SET(CMAKE_CXX_FLAGS_COVERAGE
    ${COVERAGE_COMPILER_FLAGS}
    CACHE STRING "Flags used by the C++ compiler during coverage builds." FORCE)
SET(CMAKE_C_FLAGS_COVERAGE
    ${COVERAGE_COMPILER_FLAGS}
    CACHE STRING "Flags used by the C compiler during coverage builds." FORCE)
SET(CMAKE_EXE_LINKER_FLAGS_COVERAGE
    ""
    CACHE STRING "Flags used for linking binaries during coverage builds." FORCE)
SET(CMAKE_SHARED_LINKER_FLAGS_COVERAGE
    ""
    CACHE STRING "Flags used by the shared libraries linker during coverage builds." FORCE)
MARK_AS_ADVANCED(CMAKE_CXX_FLAGS_COVERAGE CMAKE_C_FLAGS_COVERAGE CMAKE_EXE_LINKER_FLAGS_COVERAGE
                 CMAKE_SHARED_LINKER_FLAGS_COVERAGE)

IF(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    MESSAGE(WARNING "Code coverage results with an optimised (non-Debug) build may be misleading")
ENDIF() # NOT CMAKE_BUILD_TYPE STREQUAL "Debug"

IF(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    LINK_LIBRARIES(gcov)
ELSE()
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
ENDIF()

# Defines a target for running and collection code coverage information
# Builds dependencies, runs the given executable and outputs reports.
# NOTE! The executable should always have a ZERO as exit code otherwise
# the coverage generation will not complete.
#
# SETUP_TARGET_FOR_COVERAGE_GCOVR(
#     NAME ctest_coverage                    # New target name
#     EXECUTABLE ctest -j ${PROCESSOR_COUNT} # Executable in PROJECT_BINARY_DIR
#     DEPENDENCIES executable_target         # Dependencies to build first
# )
FUNCTION(SETUP_TARGET_FOR_COVERAGE_GCOVR)

    SET(options NONE)
    SET(oneValueArgs NAME)
    SET(multiValueArgs EXECUTABLE EXECUTABLE_ARGS DEPENDENCIES)
    CMAKE_PARSE_ARGUMENTS(Coverage "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    IF(NOT SIMPLE_PYTHON_EXECUTABLE)
        MESSAGE(FATAL_ERROR "python not found! Aborting...")
    ENDIF() # NOT SIMPLE_PYTHON_EXECUTABLE

    IF(NOT GCOVR_PATH)
        MESSAGE(FATAL_ERROR "gcovr not found! Aborting...")
    ENDIF() # NOT GCOVR_PATH

    # Combine excludes to several -e arguments
    SET(GCOVR_EXCLUDES "")
    FOREACH(EXCLUDE ${COVERAGE_GCOVR_EXCLUDES})
        LIST(APPEND GCOVR_EXCLUDES "-e")
        LIST(APPEND GCOVR_EXCLUDES "${EXCLUDE}")
    ENDFOREACH()

    ADD_CUSTOM_TARGET(
        ${Coverage_NAME}
        # Run tests
        ${Coverage_EXECUTABLE} ${Coverage_EXECUTABLE_ARGS}
        # Running gcovr
        COMMAND ${GCOVR_PATH} -r ${PROJECT_SOURCE_DIR} ${GCOVR_EXCLUDES} --object-directory=${PROJECT_BINARY_DIR}
                --merge-mode-functions=merge-use-line-min
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        DEPENDS ${Coverage_DEPENDENCIES}
        COMMENT "Running gcovr to produce Cobertura code coverage report.")

    # Show info where to find the report
    ADD_CUSTOM_COMMAND(
        TARGET ${Coverage_NAME}
        POST_BUILD
        COMMAND ;
        COMMENT "Finished creation of Cobertura code coverage report.")

ENDFUNCTION() # SETUP_TARGET_FOR_COVERAGE_GCOVR

# Defines a target for running and collection code coverage information
# Builds dependencies, runs the given executable and outputs reports.
# NOTE! The executable should always have a ZERO as exit code otherwise
# the coverage generation will not complete.
#
# SETUP_TARGET_FOR_COVERAGE_GCOVR_HTML(
#     NAME ctest_coverage                    # New target name
#     EXECUTABLE ctest -j ${PROCESSOR_COUNT} # Executable in PROJECT_BINARY_DIR
#     DEPENDENCIES executable_target         # Dependencies to build first
# )
FUNCTION(SETUP_TARGET_FOR_COVERAGE_GCOVR_HTML)

    SET(options NONE)
    SET(oneValueArgs NAME)
    SET(multiValueArgs EXECUTABLE EXECUTABLE_ARGS DEPENDENCIES)
    CMAKE_PARSE_ARGUMENTS(Coverage "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    IF(NOT SIMPLE_PYTHON_EXECUTABLE)
        MESSAGE(FATAL_ERROR "python not found! Aborting...")
    ENDIF() # NOT SIMPLE_PYTHON_EXECUTABLE

    IF(NOT GCOVR_PATH)
        MESSAGE(FATAL_ERROR "gcovr not found! Aborting...")
    ENDIF() # NOT GCOVR_PATH

    # Combine excludes to several -e arguments
    SET(GCOVR_EXCLUDES "")
    FOREACH(EXCLUDE ${COVERAGE_GCOVR_EXCLUDES})
        LIST(APPEND GCOVR_EXCLUDES "-e")
        LIST(APPEND GCOVR_EXCLUDES "${EXCLUDE}")
    ENDFOREACH()

    ADD_CUSTOM_TARGET(
        ${Coverage_NAME}
        # Run tests
        ${Coverage_EXECUTABLE} ${Coverage_EXECUTABLE_ARGS}
        # Create folder
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/${Coverage_NAME}
        # Running gcovr
        COMMAND ${GCOVR_PATH} --html --html-details -r ${PROJECT_SOURCE_DIR} ${GCOVR_EXCLUDES}
                --object-directory=${PROJECT_BINARY_DIR}  --merge-mode-functions=merge-use-line-min
                -o ${Coverage_NAME}/index.html
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        DEPENDS ${Coverage_DEPENDENCIES}
        COMMENT "Running gcovr to produce HTML code coverage report.")

    # Show info where to find the report
    ADD_CUSTOM_COMMAND(
        TARGET ${Coverage_NAME}
        POST_BUILD
        COMMAND ;
        COMMENT "Open ./${Coverage_NAME}/index.html in your browser to view the coverage report.")

ENDFUNCTION() # SETUP_TARGET_FOR_COVERAGE_GCOVR_HTML

FUNCTION(APPEND_COVERAGE_COMPILER_FLAGS)
    SET(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} ${COVERAGE_COMPILER_FLAGS}"
        PARENT_SCOPE)
    SET(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} ${COVERAGE_COMPILER_FLAGS}"
        PARENT_SCOPE)
    MESSAGE(STATUS "Appending code coverage compiler flags: ${COVERAGE_COMPILER_FLAGS}")
ENDFUNCTION() # APPEND_COVERAGE_COMPILER_FLAGS
