# Define module
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Geant4 is required for geometry description and charge deposition.
FIND_PACKAGE(Geant4 REQUIRED vis_all ui_all)
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

# Include Geant4 directories (NOTE Geant4_USE_FILE is not used!)
INCLUDE_DIRECTORIES(SYSTEM ${Geant4_INCLUDE_DIRS})

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME}
    VisualizationGeant4Module.cpp
)

# Add Geant4 libraries
TARGET_LINK_LIBRARIES(${MODULE_NAME} ${Geant4_LIBRARIES})

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
