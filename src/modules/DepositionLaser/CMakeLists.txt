# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Define module and return the generated name as MODULE_NAME
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Prefer local data files if not installed to other location
GET_FILENAME_COMPONENT(ABSOLUTE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} ABSOLUTE)
IF(ABSOLUTE_INSTALL_PREFIX STREQUAL PROJECT_SOURCE_DIR AND NOT DEFINED ENV{ALLPIX_BUILD_ENV})
    # Use local data directory (unless other is given)
    SET(LASER_DATA_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/data"
        CACHE PATH "Laser data directory" FORCE)
ELSE()
    # Set the install path to share
    SET(LASER_DATA_DIRECTORY
        "share/${CMAKE_PROJECT_NAME}/data"
        CACHE PATH "Laser data directory" FORCE)
ENDIF()

SET(LASER_DATA_INSTALL_DIRECTORY "${LASER_DATA_DIRECTORY}")
IF(NOT IS_ABSOLUTE ${LASER_DATA_DIRECTORY})
    SET(LASER_DATA_DIRECTORY
        "${CMAKE_INSTALL_PREFIX}/${LASER_DATA_DIRECTORY}"
        CACHE PATH "Laser data directory" FORCE)
ENDIF()

# Install the files to the data directory
# NOTE: With default install path this does not change anything
INSTALL(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/
    DESTINATION ${LASER_DATA_INSTALL_DIRECTORY}
    FILES_MATCHING
    PATTERN "*.data")

ADD_DEFINITIONS(-DALLPIX_LASER_DATA_DIRECTORY="${LASER_DATA_DIRECTORY}")


# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} DepositionLaserModule.cpp)

# Register module tests
ALLPIX_MODULE_TESTS(${MODULE_NAME} "tests")

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
