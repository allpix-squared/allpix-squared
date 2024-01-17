# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Define module and return the generated name as MODULE_NAME
ALLPIX_DETECTOR_MODULE(MODULE_NAME)

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} ProjectionPropagationModule.cpp)

# Register module tests
ALLPIX_MODULE_TESTS(${MODULE_NAME} "tests")

TARGET_LINK_LIBRARIES(${MODULE_NAME} ROOT::Graf3d)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
