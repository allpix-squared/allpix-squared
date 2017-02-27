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
    message(${ROOT_LIBRARIES})
    TARGET_LINK_LIBRARIES(${dir} ${ROOT_LIBRARIES} ${ROOT_COMPONENT_LIBRARIES})

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
IF(ROOT_FOUND)
    # FIXME: determine this per project
    ##LINK_LIBRARIES(${ROOT_LIBRARIES} ${ROOT_COMPONENT_LIBRARIES})
    link_directories(${ROOT_LIBRARY_DIR})
ELSE(ROOT_FOUND)
    MESSAGE(FATAL_ERROR "Could not find ROOT, make sure to source the ROOT environment\n"
    "$ source YOUR_ROOT_DIR/bin/thisroot.sh")
ENDIF(ROOT_FOUND)

# Geant4 is required for geometry description and charge deposition.
# Check first, if CMake finds a "FindGeant4" module or warn the user otherwise
CheckHasModule(Geant4)
IF(HAS_MODULE_Geant4)
    FIND_PACKAGE(Geant4 REQUIRED ui_all)
ELSE(HAS_MODULE_Geant4)
    MESSAGE(FATAL_ERROR "Could not find Geant4, make sure to source the Geant4 environment\n"
    "$ source YOUR_GEANT4_DIR/bin/geant4.sh")
ENDIF(HAS_MODULE_Geant4)

###################################
# Build the required modules      #
###################################

SET(CMAKE_CXX_FLAGS "-std=c++14 -O2 -Wall -Wextra -pedantic -Wfatal-errors ${CMAKE_CXX_FLAGS}")

# Example module implementation - check this directory as a starting point for your own module implementation
option_build_module(geometry_test)
