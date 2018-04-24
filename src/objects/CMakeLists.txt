# Include the standard dependencies
INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})

# Find the RootNewMacros.cmake file and include it
GET_FILENAME_COMPONENT(ROOT_CMAKE_DIR ${ROOT_USE_FILE} DIRECTORY)
FIND_FILE(ROOT_MACROS_FILE
          NAMES RootNewMacros.cmake
          HINTS ${ROOT_CMAKE_DIR}/modules/ $ENV{ROOTSYS}/cmake/modules)

IF(NOT ROOT_MACROS_FILE)
    MESSAGE(WARNING "Cannot find ROOT macros, including generic file as fallback.")
    INCLUDE(${ROOT_USE_FILE})
ELSE()
    INCLUDE(${ROOT_MACROS_FILE})
ENDIF()

# Generate the ROOT dictionary
ROOT_GENERATE_DICTIONARY(AllpixObjectsDictionary
    ${CMAKE_CURRENT_SOURCE_DIR}/Object.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SensorCharge.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PropagatedCharge.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DepositedCharge.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PixelCharge.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Pixel.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PixelHit.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MCParticle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MCTrack.hpp
    LINKDEF
    ${CMAKE_CURRENT_SOURCE_DIR}/Linkdef.h
    OPTIONS
    -Wno-inconsistent-missing-override
    -inlineInputHeader
    -I${CMAKE_CURRENT_SOURCE_DIR}
    MODULE
    AllpixObjects
)

# Explicitly add all the dependent include dirs to the manual command
FOREACH(dir IN ITEMS ${ALLPIX_DEPS_INCLUDE_DIRS})
    IF(NOT INCLUDE_DIR_ARGS)
        SET(INCLUDE_DIR_ARGS "-I${dir}")
    ELSE()
        SET(INCLUDE_DIR_ARGS "${INCLUDE_DIR_ARGS} -I${dir}")
    ENDIF()
ENDFOREACH()

# Compile the dictionary through a special target
# WARNING This hack is necessary to prevent standard warnings and those in clang-tidy for use in the CI
ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/AllpixObjectsDictionary.cxx.o COMMAND ${CMAKE_CXX_COMPILER} -fPIC -std=c++${CMAKE_CXX_STANDARD} -I${CMAKE_SOURCE_DIR}/src ${INCLUDE_DIR_ARGS} -o ${CMAKE_CURRENT_BINARY_DIR}/AllpixObjectsDictionary.cxx.o -c ${CMAKE_CURRENT_BINARY_DIR}/AllpixObjectsDictionary.cxx DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/AllpixObjectsDictionary.cxx)

# Define the library adding the object file created above
ADD_LIBRARY(AllpixObjects SHARED
    Object.cpp
    Pixel.cpp
    SensorCharge.cpp
    PixelCharge.cpp
    DepositedCharge.cpp
    PropagatedCharge.cpp
    PixelHit.cpp
    MCParticle.cpp
    MCTrack.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AllpixObjectsDictionary.cxx.o
)

# Link the standard dependencies
TARGET_LINK_LIBRARIES(AllpixObjects ${ALLPIX_DEPS_LIBRARIES})

# Specify install for the messages
INSTALL(TARGETS AllpixObjects
    COMPONENT application
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)

# Also install the dictionary objects
INSTALL(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/libAllpixObjects_rdict.pcm
    ${CMAKE_CURRENT_BINARY_DIR}/libAllpixObjects.rootmap
    COMPONENT application
    DESTINATION lib)
