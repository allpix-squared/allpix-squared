# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Define module and return the generated name as MODULE_NAME
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Geant4 is required for geometry description and charge deposition.
FIND_PACKAGE(Geant4)
IF(NOT Geant4_FOUND)
    MESSAGE(FATAL_ERROR "Could not find Geant4, make sure to source the Geant4 environment\n"
                        "$ source YOUR_GEANT4_DIR/bin/geant4.sh")
ENDIF()

# Add "geant4.sh" as runtime dependency for setup.sh file:
ADD_RUNTIME_DEP(geant4.sh)

# Add Geant4 flags before our own flags
ADD_DEFINITIONS(${Geant4_DEFINITIONS})
SET(CMAKE_CXX_FLAGS "${Geant4_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")
IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    SET(CMAKE_CXX_FLAGS "${Geant4_CXX_FLAGS_DEBUG} ${CMAKE_CXX_FLAGS}")
ELSEIF(CMAKE_BUILD_TYPE MATCHES RELEASE)
    SET(CMAKE_CXX_FLAGS "${Geant4_CXX_FLAGS_RELEASE} ${CMAKE_CXX_FLAGS}")
ENDIF()

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} DepositionGeneratorModule.cpp PrimariesGeneratorAction.cpp PrimariesReaderGenie.cpp)

# To support HepMC data format the HepMC3 package is required
FIND_PACKAGE(HepMC3 QUIET)
IF(HepMC3_FOUND)
    TARGET_SOURCES(${MODULE_NAME} PRIVATE PrimariesReaderHepMC.cpp)
    TARGET_COMPILE_DEFINITIONS(${MODULE_NAME} PRIVATE ALLPIX_GENERATOR_HEPMC=1)
    TARGET_INCLUDE_DIRECTORIES(${MODULE_NAME} SYSTEM PRIVATE ${HEPMC3_INCLUDE_DIR})
    TARGET_LINK_LIBRARIES(${MODULE_NAME} ${Geant4_LIBRARIES} ${HEPMC3_LIBRARIES})
    MESSAGE(STATUS "  Found HepMC3, building HepMC3 interface")
ELSE()
    MESSAGE(STATUS "  HepMC3 not found, not building HepMC3 interface")
    TARGET_COMPILE_DEFINITIONS(${MODULE_NAME} PRIVATE ALLPIX_GENERATOR_HEPMC=0)
ENDIF()

# Include Geant4 directories (NOTE Geant4_USE_FILE is not used!)
TARGET_INCLUDE_DIRECTORIES(${MODULE_NAME} SYSTEM PRIVATE ${Geant4_INCLUDE_DIRS})

# Add Geant4 libraries
TARGET_LINK_LIBRARIES(${MODULE_NAME} ${Geant4_LIBRARIES} AllpixModuleDepositionGeant4 ROOT::Tree ROOT::TreePlayer)

# Allpix Geant4 interface is required for this module
ALLPIX_MODULE_REQUIRE_GEANT4_INTERFACE(${MODULE_NAME})

# Register module tests
ALLPIX_MODULE_TESTS(${MODULE_NAME} "tests")

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
