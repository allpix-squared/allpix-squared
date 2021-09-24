# Define module and return the generated name as MODULE_NAME
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Prefer local data files if not installed to other location
GET_FILENAME_COMPONENT(ABSOLUTE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} ABSOLUTE)
IF(ABSOLUTE_INSTALL_PREFIX STREQUAL PROJECT_SOURCE_DIR)
    # Use local data directory (unless other is given)
    SET(CRY_DATA_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/data"
    CACHE PATH "CRY data directory" FORCE)
ELSE()
    # Set the install path to share
    SET(CRY_DATA_DIRECTORY
        "share/${CMAKE_PROJECT_NAME}/data"
    CACHE PATH "CRY data directory" FORCE)
ENDIF()

SET(DATA_INSTALL_DIRECTORY "${CRY_DATA_DIRECTORY}")
IF(NOT IS_ABSOLUTE ${CRY_DATA_DIRECTORY})
    SET(CRY_DATA_DIRECTORY
        "${CMAKE_INSTALL_PREFIX}/${CRY_DATA_DIRECTORY}"
    CACHE PATH "CRY data directory" FORCE)
ENDIF()

# Install the files to the data directory
# NOTE: With default install path this does not change anything
FILE(
    GLOB DATA_FILES
    LIST_DIRECTORIES false
    CONFIGURE_DEPENDS "*.data")
INSTALL(
    FILES ${DATA_FILES}
    DESTINATION ${CRY_DATA_DIRECTORY}
    COMPONENT data)

ADD_DEFINITIONS(-DALLPIX_CRY_DATA_DIRECTORY="${CRY_DATA_DIRECTORY}")


# Required for CMake when Geant4 has HDF5 enabled
ENABLE_LANGUAGE(C)

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
ALLPIX_MODULE_SOURCES(${MODULE_NAME}
    DepositionCosmicsModule.cpp
    CosmicsGeneratorActionG4.cpp
    RNGWrapper.cpp
    cry/CRYAbsFunction.cc
    cry/CRYAbsParameter.cc
    cry/CRYBinning.cc
    cry/CRYCosLatitudeFunction.cc
    cry/CRYData.cc
    cry/CRYFunctionDict.cc
    cry/CRYGenerator.cc
    cry/CRYParameter.cc
    cry/CRYParamI.cc
    cry/CRYParticle.cc
    cry/CRYPdf.cc
    cry/CRYPrimary.cc
    cry/CRYPrimarySpectrumFunction.cc
    cry/CRYSetup.cc
    cry/CRYUtils.cc
    cry/CRYWeightFunc.cc)

# Include Geant4 directories (NOTE Geant4_USE_FILE is not used!)
TARGET_INCLUDE_DIRECTORIES(${MODULE_NAME} SYSTEM PRIVATE ${Geant4_INCLUDE_DIRS})

# Add Geant4 libraries
TARGET_LINK_LIBRARIES(${MODULE_NAME} ${Geant4_LIBRARIES} AllpixModuleDepositionGeant4)

# Allpix Geant4 interface is required for this module
ALLPIX_MODULE_REQUIRE_GEANT4_INTERFACE(${MODULE_NAME})

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
