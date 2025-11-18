# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Optional library to interface with Geant4

# Put message
MESSAGE(STATUS "Building Geant4 Interface")

# Find Geant4
FIND_PACKAGE(Geant4)
IF(NOT Geant4_FOUND)
    MESSAGE(FATAL_ERROR "Could not find Geant4, make sure to source the Geant4 environment\n"
                        "$ source YOUR_GEANT4_DIR/bin/geant4.sh")
ENDIF()

# Add "geant4.sh" as runtime dependency for setup.sh file:
ADD_RUNTIME_DEP(geant4.sh)

# Define the library adding the object file created above
ADD_LIBRARY(AllpixGeant4Interface SHARED MTRunManager.cpp RunManager.cpp WorkerRunManager.cpp G4LoggingDestination.cpp)

# Add Geant4 flags before our own flags
ADD_DEFINITIONS(${Geant4_DEFINITIONS})
SEPARATE_ARGUMENTS(G4_CXX_FLAGS NATIVE_COMMAND ${Geant4_CXX_FLAGS})
TARGET_COMPILE_OPTIONS(AllpixGeant4Interface PRIVATE ${G4_CXX_FLAGS})
IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    SEPARATE_ARGUMENTS(G4_CXX_FLAGS_DEBUG NATIVE_COMMAND ${Geant4_CXX_FLAGS_DEBUG})
    TARGET_COMPILE_OPTIONS(AllpixGeant4Interface PRIVATE ${G4_CXX_FLAGS_DEBUG})
ELSEIF(CMAKE_BUILD_TYPE MATCHES RELEASE)
    SEPARATE_ARGUMENTS(G4_CXX_FLAGS_RELEASE NATIVE_COMMAND ${Geant4_CXX_FLAGS_RELEASE})
    TARGET_COMPILE_OPTIONS(AllpixGeant4Interface PRIVATE ${G4_CXX_FLAGS_RELEASE})
ENDIF()

# Include Geant4 directories (NOTE Geant4_USE_FILE is not used!)
TARGET_INCLUDE_DIRECTORIES(AllpixGeant4Interface SYSTEM PRIVATE ${Geant4_INCLUDE_DIRS})

# Add Geant4 libraries
TARGET_LINK_LIBRARIES(AllpixGeant4Interface ${ALLPIX_LIBRARIES} ${Geant4_LIBRARIES})

# Specify install for the messages
INSTALL(
    TARGETS AllpixGeant4Interface
    COMPONENT application
    EXPORT Allpix
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib OPTIONAL)
