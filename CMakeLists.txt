########################################################
# CMake file for the allpix2 framework
CMAKE_MINIMUM_REQUIRED(VERSION 3.4.3 FATAL_ERROR)
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
SET(CMAKE_MACOSX_RPATH 1)
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
# Define build flags for allpix   #
###################################

# Set standard build flags
SET(COMPILER_FLAGS -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wconversion -Wzero-as-null-pointer-constant -Wuseless-cast -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -Werror -Wshadow -Wformat-security -Wdeprecated -fdiagnostics-color=auto -Weffc++)

# Check for supported flags and remove unsupported warnings
INCLUDE(CheckCXXCompilerFlag)
FOREACH( FLAG ${COMPILER_FLAGS} )
    STRING(REPLACE "-" "_" FLAG_WORD ${FLAG} )
    STRING(REPLACE "+" "P" FLAG_WORD ${FLAG_WORD} )

    CHECK_CXX_COMPILER_FLAG( "${FLAG}" CXX_FLAG_WORKS_${FLAG_WORD} )
    IF( ${CXX_FLAG_WORKS_${FLAG_WORD}} )
      SET ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")
    ELSE()
      MESSAGE ( STATUS "NOT adding ${FLAG} to CXX_FLAGS - unsupported flag" )
    ENDIF()
ENDFOREACH()

message(${CMAKE_CXX_FLAGS})

# Require a C++14 compiler without extensions
SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

###################################
# Define build targets for allpix #
###################################

# Always build main allpix library
ADD_SUBDIRECTORY(src/core)

# Build messages
ADD_SUBDIRECTORY(src/messages)

# Build required modules
ADD_SUBDIRECTORY(src/modules)

#IF(BUILD_spice)
#  ADD_SUBDIRECTORY(interfaces/spice)
#ENDIF(BUILD_spice)

# Add targets for Doxygen code reference and LaTeX User manual
ADD_SUBDIRECTORY(doc)

# BUILD EXECUTABLE 
ADD_SUBDIRECTORY(src/exec)
