# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Define module and return the generated name as MODULE_NAME
ALLPIX_DETECTOR_MODULE(MODULE_NAME)

# LCIO library is required for LCIO file writing, obviously
FIND_PACKAGE(Libapx 0.1 REQUIRED)
ADD_RUNTIME_LIB(APX::APX)

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} PixESLWriterModule.cpp)
TARGET_LINK_LIBRARIES(${MODULE_NAME} APX::APX)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
