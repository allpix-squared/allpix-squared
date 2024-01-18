# SPDX-FileCopyrightText: 2021-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Define module and return the generated name as MODULE_NAME
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# The tabulated data is downloaded at compile time since the files are too big for the repository
INCLUDE(ExternalData)
# cmake-lint: disable=C0103
# Define resources for external data, copy from EOS directly if available
SET(ExternalData_URL_TEMPLATES "file:///eos/project-a/allpix-squared/www/data/%(algo)/%(hash)"
                               "https://project-allpix-squared.web.cern.ch/data/%(algo)/%(hash)")

# Make copies, not symlinks so that the files can be picked up and installed by the install target
SET(ExternalData_NO_SYMLINKS 1)

# Register the data files via placeholders with their name, the extension .md5 and their MD5 has as content:
EXTERNALDATA_EXPAND_ARGUMENTS(fetch_cry_data _data_file DATA{data/cosmics_0.data})
EXTERNALDATA_EXPAND_ARGUMENTS(fetch_cry_data _data_file DATA{data/cosmics_2100.data})
EXTERNALDATA_EXPAND_ARGUMENTS(fetch_cry_data _data_file DATA{data/cosmics_11300.data})

# Set up a CMake target for downloading the files, exclude from "make all" but make this module target depend on it:
EXTERNALDATA_ADD_TARGET(fetch_cry_data)
SET_TARGET_PROPERTIES(fetch_cry_data PROPERTIES EXCLUDE_FROM_ALL TRUE)
ADD_DEPENDENCIES(${MODULE_NAME} fetch_cry_data)

# Prefer local data files if not installed to other location
GET_FILENAME_COMPONENT(ABSOLUTE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} ABSOLUTE)
IF(ABSOLUTE_INSTALL_PREFIX STREQUAL PROJECT_SOURCE_DIR AND NOT DEFINED ENV{ALLPIX_BUILD_ENV})
    # Use local data directory (unless other is given)
    SET(CRY_DATA_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/data"
        CACHE PATH "CRY data directory" FORCE)
ELSE()
    # Set the install path to share
    SET(CRY_DATA_DIRECTORY
        "share/${CMAKE_PROJECT_NAME}/data"
        CACHE PATH "CRY data directory" FORCE)
ENDIF()

SET(CRY_DATA_INSTALL_DIRECTORY "${CRY_DATA_DIRECTORY}")
IF(NOT IS_ABSOLUTE ${CRY_DATA_DIRECTORY})
    SET(CRY_DATA_DIRECTORY
        "${CMAKE_INSTALL_PREFIX}/${CRY_DATA_DIRECTORY}"
        CACHE PATH "CRY data directory" FORCE)
ENDIF()

# Install the files to the data directory
# NOTE: With default install path this does not change anything
INSTALL(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/
    DESTINATION ${CRY_DATA_INSTALL_DIRECTORY}
    FILES_MATCHING
    PATTERN "*.data")

ADD_DEFINITIONS(-DALLPIX_CRY_DATA_DIRECTORY="${CRY_DATA_DIRECTORY}")

# Add source files to library
ALLPIX_MODULE_SOURCES(
    ${MODULE_NAME}
    DepositionCosmicsModule.cpp
    CosmicsGeneratorActionG4.cpp
    RNGWrapper.cpp
    cry/CRYAbsFunction.cc
    cry/CRYAbsParameter.cc
    cry/CRYBinning.cc
    cry/CRYCosLatitudeFunction.cc
    cry/CRYData.cc
    cry/CRYFunctionDict.cc
    cry/CRYGenerator.cc
    cry/CRYParameter.cc
    cry/CRYParamI.cc
    cry/CRYParticle.cc
    cry/CRYPdf.cc
    cry/CRYPrimary.cc
    cry/CRYPrimarySpectrumFunction.cc
    cry/CRYSetup.cc
    cry/CRYUtils.cc
    cry/CRYWeightFunc.cc)

TARGET_INCLUDE_DIRECTORIES(${MODULE_NAME} SYSTEM PRIVATE "cry")

# Link to parent module library
TARGET_LINK_LIBRARIES(${MODULE_NAME} AllpixModuleDepositionGeant4)

# Allpix Geant4 interface is required for this module
ALLPIX_MODULE_REQUIRE_GEANT4_INTERFACE(${MODULE_NAME} REQUIRED)

# Register module tests
ALLPIX_MODULE_TESTS(${MODULE_NAME} "tests")

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
