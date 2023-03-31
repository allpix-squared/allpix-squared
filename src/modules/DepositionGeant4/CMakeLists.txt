# SPDX-FileCopyrightText: 2017-2023 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Define module
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Set variable to microelectronics example folder
set(MICROELEC $ENV{GEANT4_DATA_DIR}/../examples/advanced/microelectronics)

# Add source files to library
ALLPIX_MODULE_SOURCES(
    ${MODULE_NAME}
    DepositionGeant4Module.cpp
    GeneratorActionG4.cpp
    SensitiveDetectorActionG4.cpp
    TrackInfoG4.cpp
    TrackInfoManager.cpp
    SetTrackInfoUserHookG4.cpp
    SDAndFieldConstruction.cpp
    ${MICROELEC}/src/MicroElecSiPhysics.cc
    ${MICROELEC}/src/ElectronCapture.cc)

# Add includes from the microelectronics example
TARGET_INCLUDE_DIRECTORIES(${MODULE_NAME} SYSTEM PRIVATE ${MICROELEC}/include)

# Allpix Geant4 interface is required for this module
ALLPIX_MODULE_REQUIRE_GEANT4_INTERFACE(${MODULE_NAME} REQUIRED)

# Register module tests
ALLPIX_MODULE_TESTS(${MODULE_NAME} "tests")

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
