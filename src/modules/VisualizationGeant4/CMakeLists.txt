# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Define module
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} VisualizationGeant4Module.cpp)

# Allpix Geant4 interface is required for this module
ALLPIX_MODULE_REQUIRE_GEANT4_INTERFACE(${MODULE_NAME} REQUIRED COMPONENTS vis_all ui_all)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
