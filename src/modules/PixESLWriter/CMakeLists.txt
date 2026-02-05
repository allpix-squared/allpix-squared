# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

ALLPIX_ENABLE_DEFAULT(ON)

# Define module and return the generated name as MODULE_NAME
ALLPIX_DETECTOR_MODULE(MODULE_NAME)

# Switch off tests of libAPX
SET(APX_BUILD_TESTS OFF CACHE STRING "" FORCE)
set(APX_DISABLE_STRICT_OVERFLOW ON CACHE BOOL "" FORCE)

# libAPX is required
FIND_PACKAGE(Libapx QUIET 0.4.9)
IF(NOT Libapx_FOUND)
    MESSAGE(STATUS "Did not find local installation of Libapx, fetching dependency...")
    INCLUDE(FetchContent)
    FetchContent_Declare(
      Libapx
      GIT_REPOSITORY https://gitlab.cern.ch/allpix-squared/libapx.git
      GIT_TAG        b1c288397a7c24326b88199ac440dcec07c65795
      FIND_PACKAGE_ARGS 0.4.9

    )
    FetchContent_MakeAvailable(Libapx)
ELSE()
    MESSAGE(STATUS "Found local installation of Libapx")
ENDIF()

# Add source files to library
ADD_RUNTIME_LIB(Libapx::APX)
ALLPIX_MODULE_SOURCES(${MODULE_NAME} PixESLWriterModule.cpp)
TARGET_LINK_LIBRARIES(${MODULE_NAME} Libapx::APX)

# Register module tests
ALLPIX_MODULE_TESTS(${MODULE_NAME} "tests")

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
