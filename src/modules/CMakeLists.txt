# For every module, build a separate library to be loaded by allpix core

MACRO(option_build_module dir)
    #IF(${dir} MATCHES "dummy")
    OPTION(BUILD_${dir} "Build module in directory ${dir}?" ON)
    #ELSE()
    #    OPTION(BUILD_${dir} "Build module in directory ${dir}?" OFF)
    #ENDIF()
    
  IF(BUILD_${dir} OR BUILD_allmodules)
    MESSAGE( STATUS "Building module: " ${dir} )

    INCLUDE_DIRECTORIES(${dir})
    FILE(GLOB_RECURSE LIB_SOURCE_FILES "${dir}/*.cpp")
    MESSAGE( STATUS " -- sources: " ${LIB_SOURCE_FILES} )

    # FIXME: determine per module if we want to include ROOT + G4
    INCLUDE_DIRECTORIES(SYSTEM ${ROOT_INCLUDE_DIRS})
    INCLUDE(${Geant4_USE_FILE})
    
    ADD_LIBRARY(${dir} SHARED ${LIB_SOURCE_FILES})

    TARGET_LINK_LIBRARIES(${dir} AllpixCore)
    
    # FIXME: determine per module the libraries to link
    TARGET_LINK_LIBRARIES(${dir} ${ROOT_LIBRARIES} ${ROOT_COMPONENT_LIBRARIES} ${Geant4_LIBRARIES})

    INSTALL(TARGETS ${dir}
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib)
  ENDIF()
ENDMACRO()

# Option to build all modules
OPTION(BUILD_allmodules "Build all modules?" OFF)

# WARNING: THIS PREREQUISITE LOGIC SHOULD BE RESTRUCTURED

###################################
# Prerequisistes for modules      #
###################################

# ROOT is required for histogramming
FIND_PACKAGE(ROOT REQUIRED COMPONENTS Geom XMLParser)
IF(NOT ROOT_FOUND)
    MESSAGE(FATAL_ERROR "Could not find ROOT, make sure to source the ROOT environment\n"
    "$ source YOUR_ROOT_DIR/bin/thisroot.sh")
ENDIF(NOT ROOT_FOUND)

# Geant4 is required for geometry description and charge deposition.
FIND_PACKAGE(Geant4 REQUIRED ui_all vis_all)
IF(NOT Geant4_FOUND)
    MESSAGE(FATAL_ERROR "Could not find Geant4, make sure to source the Geant4 environment\n"
    "$ source YOUR_GEANT4_DIR/bin/geant4.sh")
ENDIF(NOT Geant4_FOUND)

###################################
# Build the required modules      #
###################################

set (CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS "-O2 -Wall -Wextra -pedantic -Wfatal-errors ${CMAKE_CXX_FLAGS}")

# Example module implementation - check this directory as a starting point for your own module implementation
option_build_module(geometry_test)
