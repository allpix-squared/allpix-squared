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
    MESSAGE( STATUS " -- sources: ${LIB_SOURCE_FILES}")

    # FIXME: determine per module if we want to include ROOT + G4
    INCLUDE_DIRECTORIES(SYSTEM ${ROOT_INCLUDE_DIRS})
    INCLUDE(${Geant4_USE_FILE})

    # WARNING: RESET THE CXX CMAKE_CXX_FLAGS WITH GEANT4 OPTIONS
    SET(CMAKE_CXX_FLAGS "${SAVE_CXX_FLAGS} -DG4USE_STD11")

    # ADD DEFINITIONS FOR EXPERIMENTAL EIGEN CONVERSIONS
    # ADD_DEFINITIONS(-DHAVE_ROOT -DHAVE_GEANT)

    ADD_LIBRARY(${dir} SHARED ${LIB_SOURCE_FILES})

    TARGET_LINK_LIBRARIES(${dir} AllpixCore)
    TARGET_LINK_LIBRARIES(${dir} AllpixMessages)

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

###################################
# Prerequisistes for modules      #
###################################

# WARNING: THIS PREREQUISITE LOGIC SHOULD BE RESTRUCTURED
# Geant4 is required for geometry description and charge deposition.
FIND_PACKAGE(Geant4 REQUIRED ui_all vis_all)
IF(NOT Geant4_FOUND)
    MESSAGE(FATAL_ERROR "Could not find Geant4, make sure to source the Geant4 environment\n"
    "$ source YOUR_GEANT4_DIR/bin/geant4.sh")
ENDIF(NOT Geant4_FOUND)

# Save the CXX flags because Geant4 unfortunately changes those so we have to set it back later
SET(SAVE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

###################################
# Build the required modules      #
###################################

# example module
option_build_module(example)

# geometry
option_build_module(geometry_test)
option_build_module(geometry_tgeo)

# deposition
option_build_module(deposition_simple)

# output writers
option_build_module(deposit_reader_test)
option_build_module(detector_histogrammer_test)

# visualization
option_build_module(visualization_test)
