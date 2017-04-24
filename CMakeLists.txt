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

# Access the project name (for install locations) in the source
ADD_DEFINITIONS(-DALLPIX_PROJECT_NAME="${CMAKE_PROJECT_NAME}")

# Print the version information
PRINT_VERSION()

#########################
# allpix2 build options #
#########################

# OPTION(BUILD_spice  "Compile spice library interface?" OFF)

###############################
# Setup the build environment #
###############################

# Additional packages to be searched for by cmake
LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Configure the installation prefix to allow both local as system-wide installation
SET(INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "Prefix prepended to install directories")
SET(CMAKE_INSTALL_PREFIX "${INSTALL_PREFIX}" CACHE INTERNAL "Prefix prepended to install directories" FORCE)

# Set up the RPATH so executables find the libraries even when installed in non-default location
SET(CMAKE_MACOSX_RPATH TRUE)
SET(CMAKE_SKIP_BUILD_RPATH FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

# Add the automatically determined parts of the RPATH which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# The RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" IsSystemDir)
IF("${IsSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${IsSystemDir}" STREQUAL "-1")

###################################
# Define build flags for allpix   #
###################################

# Set standard build flags
SET(COMPILER_FLAGS -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wconversion -Wzero-as-null-pointer-constant -Wuseless-cast -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -Werror -Wshadow -Wformat-security -Wdeprecated -fdiagnostics-color=auto -Wheader-hygiene)

# Require a C++14 compiler but do allow extensions
SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

# Include Threads (not used yet)
FIND_PACKAGE(Threads REQUIRED)

INCLUDE("cmake/compiler-flag-checks.cmake")

# Set some debug flags 
# FIXME: not using the flag checker now because it wrongly rejects a sanitizer flag..
IF(CMAKE_BUILD_TYPE MATCHES DEBUG_ASAN AND ((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")))
    MESSAGE(STATUS "Running debug build - Adding extra sanitizer flags")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")
    # FIXME: remove earlier shared linker flags, because they can break the sanitizer debug build
    SET(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=address -fsanitize=undefined")
ENDIF()

###################################
# Prerequisistes for allpix       #
###################################

# Define the libraries
SET(ALLPIX_LIBRARIES "")

# ROOT is required for vector etc
FIND_PACKAGE(ROOT REQUIRED COMPONENTS Geom GenVector)
IF(NOT ROOT_FOUND)
    MESSAGE(FATAL_ERROR "Could not find ROOT, make sure to source the ROOT environment\n"
    "$ source YOUR_ROOT_DIR/bin/thisroot.sh")
ENDIF(NOT ROOT_FOUND)

# Set the dependencies
SET(ALLPIX_DEPS_INCLUDE_DIRS ${ROOT_INCLUDE_DIRS})
SET(ALLPIX_DEPS_LIBRARIES ${ROOT_LIBRARIES} ${ROOT_COMPONENT_LIBRARIES})

# Include allpix cmake functions
INCLUDE("cmake/allpix.cmake")

###################################
# Load cpp format and check tools #
###################################

# Set the source files to clang-format (FIXME: determine this better)
FILE(GLOB_RECURSE
     CHECK_CXX_SOURCE_FILES
        src/*.[ch]pp src/*.h
     )
INCLUDE("cmake/clang-cpp-checks.cmake")

###################################
# Define build targets for allpix #
###################################

# Always include sources from top directory
INCLUDE_DIRECTORIES(src)

# Set the models directory and provide install if necessary
ADD_SUBDIRECTORY(models)

# Build core allpix library
ADD_SUBDIRECTORY(src/core)
SET(ALLPIX_LIBRARIES ${ALLPIX_LIBRARIES} AllpixCore)

# Build objects library
ADD_SUBDIRECTORY(src/objects)
SET(ALLPIX_LIBRARIES ${ALLPIX_LIBRARIES} AllpixObjects)

# Build required modules
ADD_SUBDIRECTORY(src/modules)

# Build the executable 
ADD_SUBDIRECTORY(src/exec)

# Add targets for Doxygen code reference and LaTeX User manual
ADD_SUBDIRECTORY(doc)
