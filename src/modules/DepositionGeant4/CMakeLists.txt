# SPDX-FileCopyrightText: 2017-2023 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Define module
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Add source files to library
ALLPIX_MODULE_SOURCES(
    ${MODULE_NAME}
    DepositionGeant4Module.cpp
    GeneratorActionG4.cpp
    SensitiveDetectorActionG4.cpp
    TrackInfoG4.cpp
    TrackInfoManager.cpp
    SetTrackInfoUserHookG4.cpp
    SDAndFieldConstruction.cpp)

# Allpix Geant4 interface is required for this module
ALLPIX_MODULE_REQUIRE_GEANT4_INTERFACE(${MODULE_NAME} REQUIRED)

# If available, add additional physics lists form Geant4:
IF(DEFINED ENV{GEANT4_DATA_DIR})
    # Set variable to microelectronics example folder
    SET(GEANT4_EXAMPLES_DIR $ENV{GEANT4_DATA_DIR}/../examples)
ELSE()
    FIND_PROGRAM(G4CONFIG "geant4-config")
    IF(G4CONFIG)
        EXEC_PROGRAM(${G4CONFIG} ARGS --prefix OUTPUT_VARIABLE G4_PATH)
        EXEC_PROGRAM(${G4CONFIG} ARGS --version OUTPUT_VARIABLE G4_VERSION)
        IF(EXISTS "${G4_PATH}/share/Geant4-${G4_VERSION}" AND IS_DIRECTORY "${G4_PATH}/share/Geant4-${G4_VERSION}")
            SET(GEANT4_EXAMPLES_DIR ${G4_PATH}/share/Geant4-${G4_VERSION}/examples)
        ELSEIF(EXISTS "${G4_PATH}/share/Geant4" AND IS_DIRECTORY "${G4_PATH}/share/Geant4")
            SET(GEANT4_EXAMPLES_DIR ${G4_PATH}/share/Geant4/examples)
        ENDIF()
    ENDIF()
ENDIF()

IF(DEFINED GEANT4_EXAMPLES_DIR)
    MESSAGE(STATUS "  Found Geant4 Examples, building add. physics lists")
    CONFIGURE_FILE(${GEANT4_EXAMPLES_DIR}/advanced/microelectronics/src/MicroElecSiPhysics.cc ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY)
    CONFIGURE_FILE(${GEANT4_EXAMPLES_DIR}/advanced/microelectronics/src/ElectronCapture.cc ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY)
    
    TARGET_SOURCES(${MODULE_NAME} PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/MicroElecSiPhysics.cc
        ${CMAKE_CURRENT_BINARY_DIR}/ElectronCapture.cc)
    # Add includes from the microelectronics example
    TARGET_INCLUDE_DIRECTORIES(${MODULE_NAME} SYSTEM PRIVATE ${GEANT4_EXAMPLES_DIR}/advanced/microelectronics/include)
ENDIF()

# Register module tests
ALLPIX_MODULE_TESTS(${MODULE_NAME} "tests")

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
