# SPDX-FileCopyrightText: 2024-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Define module and return the generated name as MODULE_NAME
ALLPIX_DETECTOR_MODULE(MODULE_NAME)

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} NetlistWriterModule.cpp)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
