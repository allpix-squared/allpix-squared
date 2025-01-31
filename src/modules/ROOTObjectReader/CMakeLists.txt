# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Define module
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} ROOTObjectReaderModule.cpp)

# Register module tests
ALLPIX_MODULE_TESTS(${MODULE_NAME} "tests")

TARGET_LINK_LIBRARIES(${MODULE_NAME} ROOT::Tree)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
