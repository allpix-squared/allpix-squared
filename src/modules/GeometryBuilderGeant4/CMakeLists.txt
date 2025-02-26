# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Define module
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Add source files to library
ALLPIX_MODULE_SOURCES(
    ${MODULE_NAME}
    GeometryBuilderGeant4Module.cpp
    GeometryConstructionG4.cpp
    MaterialManager.cpp
    Parameterization2DG4.cpp
    DetectorConstructionG4.cpp
    PassiveMaterialConstructionG4.cpp
    PassiveMaterialModel.cpp)

# Allpix Geant4 interface is required for this module
ALLPIX_MODULE_REQUIRE_GEANT4_INTERFACE(${MODULE_NAME} REQUIRED)

# Register module tests
ALLPIX_MODULE_TESTS(${MODULE_NAME} "tests")

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
