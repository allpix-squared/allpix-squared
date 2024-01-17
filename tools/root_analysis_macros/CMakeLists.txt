# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Only install macros if installation directory is outside the source directory
GET_FILENAME_COMPONENT(ABSOLUTE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} ABSOLUTE)
IF(ABSOLUTE_INSTALL_PREFIX STREQUAL PROJECT_SOURCE_DIR)
    # Use local macro directory (unless other is given)
    SET(MACRO_DIRECTORY
        "tools/root_analysis_macros"
        CACHE PATH "ROOT macros directory" FORCE)

    IF(NOT IS_ABSOLUTE ${MODEL_DIRECTORY})
        SET(MODEL_DIRECTORY
            "${CMAKE_INSTALL_PREFIX}/${MODEL_DIRECTORY}"
            CACHE PATH "Models directory" FORCE)
    ENDIF()
ELSE()
    # Set the install path to share
    # FIXME Allow for more fine tuned install configuration
    SET(MACRO_DIRECTORY
        "share/${CMAKE_PROJECT_NAME}/macros"
        CACHE PATH "ROOT macros directory" FORCE)
ENDIF()

# Install the files to the macros directory
# NOTE: With default install path this does not change anything
INSTALL(
    FILES constructComparisonTree.C recoverConfiguration.C remakeProject.C
    DESTINATION ${MACRO_DIRECTORY}
    COMPONENT tools)
