# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Disable default build
ALLPIX_ENABLE_DEFAULT(ON)

# Define module and return the generated name as MODULE_NAME
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

#First make the root library for the pixel class, then make the module library

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

# Generate the ROOT dictionary
ROOT_GENERATE_DICTIONARY(
    CorryvreckanWriterObjectsDictionary
    ${CMAKE_CURRENT_SOURCE_DIR}/corryvreckan/Object.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/corryvreckan/Pixel.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/corryvreckan/MCParticle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/corryvreckan/Event.hpp
    LINKDEF
    ${CMAKE_CURRENT_SOURCE_DIR}/corryvreckan/Linkdef.h
    OPTIONS
    -inlineInputHeader
    -I${CMAKE_CURRENT_SOURCE_DIR}/corryvreckan
    MODULE
    CorryvreckanWriterObjects)
SET_SOURCE_FILES_PROPERTIES(
    ${CMAKE_CURRENT_BINARY_DIR}/CorryvreckanWriterObjectsDictionary.cxx PROPERTIES GENERATED TRUE COMPILE_FLAGS
                                                                                                  "-w" # or whatever ?
)

# Define the library adding the object file created above
ADD_LIBRARY(CorryvreckanWriterObjects SHARED corryvreckan/Object.cpp corryvreckan/Pixel.cpp corryvreckan/Event.cpp
                                             ${CMAKE_CURRENT_BINARY_DIR}/CorryvreckanWriterObjectsDictionary.cxx)

# Link the standard dependencies
TARGET_LINK_LIBRARIES(CorryvreckanWriterObjects ${ALLPIX_DEPS_LIBRARIES})

# Specify install for the messages
INSTALL(
    TARGETS CorryvreckanWriterObjects
    COMPONENT modules
    EXPORT Allpix
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)

# Also install the dictionary objects
INSTALL(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/libCorryvreckanWriterObjects_rdict.pcm
          ${CMAKE_CURRENT_BINARY_DIR}/libCorryvreckanWriterObjects.rootmap
    COMPONENT modules
    DESTINATION lib)

# Add source files to module library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} CorryvreckanWriterModule.cpp)

TARGET_LINK_LIBRARIES(${MODULE_NAME} CorryvreckanWriterObjects ROOT::Tree)

# Register module tests
ALLPIX_MODULE_TESTS(${MODULE_NAME} "tests")

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
