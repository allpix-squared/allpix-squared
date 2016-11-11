########################################################
# CMake file for the allpix2 framework
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)
########################################################

# Project name
PROJECT(allpix)
INCLUDE("cmake/tools.cmake")

# Prepare version number
GetVersionString()

#########################
# allpix2 build options #
#########################

OPTION(BUILD_spice  "Compile spice library interface?" OFF)


###############################
# Setup the build environment #
###############################

# Additional packages to be searched for by cmake
LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Configure the installation prefix to allow system-wide installation
SET(INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "Prefix prepended to install directories")
SET(CMAKE_INSTALL_PREFIX "${INSTALL_PREFIX}" CACHE INTERNAL "Prefix prepended to install directories" FORCE)

# Set up the RPATH so executables find the libraries even when installed in non-default location
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
# Add the automatically determined parts of the RPATH which point to directories outside
# the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")


###################################
# Check prerequisities for allpix #
###################################

# Geant4 is required for geometry description and charge deposition.
# Check first, if CMake finds a "FindGeant4" module or warn the user otherwise
CheckHasModule(Geant4)
IF(HAS_MODULE_Geant4)
  FIND_PACKAGE(Geant4 REQUIRED)
  INCLUDE(${Geant4_USE_FILE})
ELSE(HAS_MODULE_Geant4)
  MESSAGE(FATAL_ERROR "Could not find Geant4, make sure to source the Geant4 environment\n"
    "$ source YOUR_GEANT4_DIR/bin/geant4.sh")
ENDIF(HAS_MODULE_Geant4)

# ROOT is required for histogramming
FIND_PACKAGE(ROOT REQUIRED)
IF(ROOT_FOUND)
  INCLUDE_DIRECTORIES(${ROOT_INCLUDE_DIR})
  LINK_DIRECTORIES(${ROOT_LIBRARY_DIR})
ELSE(ROOT_FOUND)
MESSAGE(FATAL_ERROR "Could not find ROOT, make sure to source the ROOT environment\n"
    "$ source YOUR_ROOT_DIR/bin/thisroot.sh")
ENDIF(ROOT_FOUND)


###################################
# Define build targets for allpix #
###################################

# Include directories
INCLUDE_DIRECTORIES(core/utils
  core/allpix
  ${GEANT4_INCLUDE_DIR})

# Always build main allpix library;
ADD_SUBDIRECTORY(core)

IF(BUILD_spice)
  ADD_SUBDIRECTORY(interfaces/spice)
ENDIF(BUILD_spice)

# Add targets for Doxygen code reference and LaTeX User manual
ADD_SUBDIRECTORY(doc)

# Add target for the main executable
ADD_EXECUTABLE(runallpix main/runallpix.cc )
TARGET_LINK_LIBRARIES(runallpix ${PROJECT_NAME} ${Geant4_LIBRARIES})

INSTALL(TARGETS runallpix
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib)
