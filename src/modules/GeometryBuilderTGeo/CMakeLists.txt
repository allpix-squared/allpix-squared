################################################
# Geometry Builder Module, TGeo Implementation #
################################################

# Define module and return the generated name as MODULE_NAME
ALLPIX_MODULE(MODULE_NAME)

# Geant4 is required for CLHEP/Units/SystemOfUnits.h
FIND_PACKAGE(Geant4 REQUIRED)
IF(NOT Geant4_FOUND)
    MESSAGE(FATAL_ERROR "Could not find Geant4, make sure to source the Geant4 environment\n"
    "$ source YOUR_GEANT4_DIR/bin/geant4.sh")
ENDIF(NOT Geant4_FOUND)

# Include Geant4 directories (WARNING: do not use the Geant4_USE_FILE!)
INCLUDE_DIRECTORIES(SYSTEM ${Geant4_INCLUDE_DIRS})

# Add library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} 
    ReadGeoDescription.cpp
    TGeoBuilderModule.cpp
)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
