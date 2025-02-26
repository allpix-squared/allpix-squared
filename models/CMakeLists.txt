# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Set the suffix for model files
SET(_MODEL_SUFFIX
    ".conf"
    CACHE INTERNAL "Model files suffix" FORCE)

# Prefer local models if not installed to other location
GET_FILENAME_COMPONENT(ABSOLUTE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} ABSOLUTE)
IF(ABSOLUTE_INSTALL_PREFIX STREQUAL PROJECT_SOURCE_DIR AND NOT DEFINED ENV{ALLPIX_BUILD_ENV})
    # Use local model directory (unless other is given)
    SET(MODEL_DIRECTORY
        "models"
        CACHE PATH "Models directory" FORCE)
ELSE()
    # Set the install path to share
    # FIXME Allow for more fine tuned install configuration
    SET(MODEL_DIRECTORY
        "share/${CMAKE_PROJECT_NAME}/models"
        CACHE PATH "Models directory" FORCE)
ENDIF()

SET(MODEL_INSTALL_DIRECTORY "${MODEL_DIRECTORY}")
IF(NOT IS_ABSOLUTE ${MODEL_DIRECTORY})
    SET(MODEL_DIRECTORY
        "${CMAKE_INSTALL_PREFIX}/${MODEL_DIRECTORY}"
        CACHE PATH "Models directory" FORCE)
ENDIF()

# Install the files to the models directory
# NOTE: With default install path this does not change anything
FILE(
    GLOB MODEL_FILES
    LIST_DIRECTORIES false
    CONFIGURE_DEPENDS "*.conf")
INSTALL(
    FILES ${MODEL_FILES}
    DESTINATION ${MODEL_INSTALL_DIRECTORY}
    COMPONENT models)
