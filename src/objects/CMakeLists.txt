# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Include the standard dependencies
INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})

# Find the RootNewMacros.cmake file and include it
GET_FILENAME_COMPONENT(ROOT_CMAKE_DIR ${ROOT_USE_FILE} DIRECTORY)
FIND_FILE(
    ROOT_MACROS_FILE
    NAMES RootNewMacros.cmake RootMacros.cmake
    HINTS ${ROOT_CMAKE_DIR} ${ROOT_CMAKE_DIR}/modules/ $ENV{ROOTSYS}/cmake/modules)

IF(NOT ROOT_MACROS_FILE)
    MESSAGE(WARNING "Cannot find ROOT macros, including generic file as fallback.")
    INCLUDE(${ROOT_USE_FILE})
ELSE()
    INCLUDE(${ROOT_MACROS_FILE})
ENDIF()

# Define the library adding the object file created above
ADD_LIBRARY(
    AllpixObjects SHARED
    Object.cpp
    Pixel.cpp
    SensorCharge.cpp
    Pulse.cpp
    PixelCharge.cpp
    DepositedCharge.cpp
    PropagatedCharge.cpp
    PixelHit.cpp
    PixelPulse.cpp
    MCParticle.cpp
    MCTrack.cpp)

# Link the standard dependencies
TARGET_LINK_LIBRARIES(AllpixObjects PUBLIC ${ALLPIX_DEPS_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(AllpixObjects PUBLIC $<INSTALL_INTERFACE:include>)

# Generate the ROOT dictionary
# NOTE these headers will be inlined in the generated dictionary. Therefore their order needs to reflect dependencies
# between them such that at the time of reference, all classes are defined already.
ROOT_GENERATE_DICTIONARY(
    AllpixObjectsDictionary
    ${CMAKE_CURRENT_SOURCE_DIR}/Object.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MCTrack.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MCParticle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SensorCharge.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DepositedCharge.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Pulse.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Pixel.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PropagatedCharge.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PixelCharge.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PixelHit.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PixelPulse.hpp
    MODULE
    AllpixObjects
    LINKDEF
    ${CMAKE_CURRENT_SOURCE_DIR}/Linkdef.h
    OPTIONS
    -inlineInputHeader
    -I${CMAKE_CURRENT_SOURCE_DIR})

SET_SOURCE_FILES_PROPERTIES(
    ${CMAKE_CURRENT_BINARY_DIR}/AllpixObjectsDictionary.cxx
    PROPERTIES GENERATED TRUE
               COMPILE_FLAGS
               "-Wno-unused-function -Wno-overlength-strings -Wno-zero-as-null-pointer-constant -w" # or whatever ?
)

# Specify install for the messages
INSTALL(
    TARGETS AllpixObjects
    EXPORT Allpix
    COMPONENT application
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)

# Also install the dictionary objects
INSTALL(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/libAllpixObjects_rdict.pcm ${CMAKE_CURRENT_BINARY_DIR}/libAllpixObjects.rootmap
    COMPONENT application
    DESTINATION lib)

INSTALL(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DESTINATION include
    PATTERN "*.hpp"
    PATTERN "*.h"
    PATTERN "Linkdef.h" EXCLUDE)
