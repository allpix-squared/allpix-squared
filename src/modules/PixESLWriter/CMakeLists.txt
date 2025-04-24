# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

ALLPIX_ENABLE_DEFAULT(ON)

# Define module and return the generated name as MODULE_NAME
ALLPIX_DETECTOR_MODULE(MODULE_NAME)

# Switch off tests of libAPX
SET(APX_BUILD_TESTS OFF CACHE INTERNAL "Turn off tests")

# libAPX is required
INCLUDE(FetchContent)
FetchContent_Declare(
  Libapx
  GIT_REPOSITORY https://gitlab.cern.ch/allpix-squared/libapx.git
  GIT_TAG        d36c69a0fa59a8b693aece23902fde34f82959a1 # v0.4.2
  FIND_PACKAGE_ARGS 0.4.2
)
FetchContent_MakeAvailable(Libapx)

# Add source files to library
ADD_RUNTIME_LIB(Libapx::APX)
ALLPIX_MODULE_SOURCES(${MODULE_NAME} PixESLWriterModule.cpp)
TARGET_LINK_LIBRARIES(${MODULE_NAME} Libapx::APX)

# Register module tests
ALLPIX_MODULE_TESTS(${MODULE_NAME} "tests")

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
