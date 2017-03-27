# Libraries that do not strictly belong to a single module but also not in the core
# FIXME: THIS IS TEMPORARY AND SHOULD LATER BE FIXED IN A BETTER WAY

# Geant4 is required for geometry description and charge deposition.
FIND_PACKAGE(Geant4 REQUIRED ui_all vis_all)
IF(NOT Geant4_FOUND)
    MESSAGE(FATAL_ERROR "Could not find Geant4, make sure to source the Geant4 environment\n"
    "$ source YOUR_GEANT4_DIR/bin/geant4.sh")
ENDIF(NOT Geant4_FOUND)

# Add Geant4 flags before our own flags
ADD_DEFINITIONS(${Geant4_DEFINITIONS})
SET(${CMAKE_CXX_FLAGS} "${Geant4_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")
IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    SET(${CMAKE_CXX_FLAGS} "${Geant4_CXX_FLAGS_DEBUG} ${CMAKE_CXX_FLAGS}")
ELSEIF(CMAKE_BUILD_TYPE MATCHES RELEASE)
    SET(${CMAKE_CXX_FLAGS} "${Geant4_CXX_FLAGS_RELEASE} ${CMAKE_CXX_FLAGS}")
ENDIF()

# Include Geant4 directories (WARNING: do not use the Geant4_USE_FILE!)
INCLUDE_DIRECTORIES(SYSTEM ${Geant4_INCLUDE_DIRS})

# include directories for dependencies
INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})

# add the library
ADD_LIBRARY(AllPixModulesCommon SHARED
    BumpsParameterizationG4.cpp
    ReadGeoDescription.cpp
)

# link the standard allpix libraries
TARGET_LINK_LIBRARIES(AllPixModulesCommon ${ALLPIX_LIBRARIES} ${ALLPIX_DEPS_LIBRARIES})

# add Geant4 libraries
TARGET_LINK_LIBRARIES(AllPixModulesCommon ${Geant4_LIBRARIES})

# add installation target
INSTALL(TARGETS AllPixModulesCommon
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
