# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Packaging configuration
SET(CPACK_PACKAGE_NAME "allpix-squared")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Generic Pixel Detector Simulation Framework")
SET(CPACK_PACKAGE_VENDOR "The Allpix Squared Authors")
SET(CPACK_PACKAGE_CONTACT "The Allpix Squared Authors <allpix.squared@cern.ch>")
SET(CPACK_PACKAGE_ICON "doc/logo_small.png")
SET(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE.md")

# Figure out, which system we are running on:
IF(DEFINED ENV{BUILD_FLAVOUR})
    SET(CPACK_SYSTEM_NAME $ENV{BUILD_FLAVOUR})
ELSE()
    SET(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}")
ENDIF()

# Set version derived from project version, or "latest" for untagged:
IF(NOT TAG_FOUND)
    SET(CPACK_PACKAGE_VERSION "latest")
ELSE()
    SET(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
ENDIF()

SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}")

# Configure the targets and components to include
SET(CPACK_GENERATOR "TGZ")
SET(CPACK_COMPONENTS_ALL application modules)
SET(CPACK_INSTALLED_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/setup/;.")
