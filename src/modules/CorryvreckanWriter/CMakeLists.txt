# Disable default build
ALLPIX_ENABLE_DEFAULT(ON)

# Define module and return the generated name as MODULE_NAME
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

#First make the root library for the pixel class, then make the module library

# Find the RootNewMacros.cmake file and include it
GET_FILENAME_COMPONENT(ROOT_CMAKE_DIR ${ROOT_USE_FILE} DIRECTORY)
FIND_FILE(ROOT_MACROS_FILE
          NAMES RootNewMacros.cmake RootMacros.cmake
          HINTS ${ROOT_CMAKE_DIR} ${ROOT_CMAKE_DIR}/modules/ $ENV{ROOTSYS}/cmake/modules)

IF(NOT ROOT_MACROS_FILE)
    MESSAGE(WARNING "Cannot find ROOT macros, including generic file as fallback.")
    INCLUDE(${ROOT_USE_FILE})
ELSE()
    INCLUDE(${ROOT_MACROS_FILE})
ENDIF()
# Geant4 is required
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

# Generate the ROOT dictionary
ROOT_GENERATE_DICTIONARY(CorryvreckanObjectsDictionary
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
    CorryvreckanObjects
)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/CorryvreckanObjectsDictionary.cxx
  PROPERTIES  GENERATED TRUE COMPILE_FLAGS "-w" # or whatever ?
)


# Define the library adding the object file created above
ADD_LIBRARY(CorryvreckanObjects SHARED
    corryvreckan/Object.cpp
    corryvreckan/Pixel.cpp
    corryvreckan/Event.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/CorryvreckanObjectsDictionary.cxx
)

# Link the standard dependencies
TARGET_LINK_LIBRARIES(CorryvreckanObjects ${ALLPIX_DEPS_LIBRARIES})

# Specify install for the messages
INSTALL(TARGETS CorryvreckanObjects
    COMPONENT modules
    EXPORT Allpix
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)

# Also install the dictionary objects
INSTALL(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/libCorryvreckanObjects_rdict.pcm
    ${CMAKE_CURRENT_BINARY_DIR}/libCorryvreckanObjects.rootmap
    COMPONENT modules
    DESTINATION lib)

# Add source files to module library
ALLPIX_MODULE_SOURCES(${MODULE_NAME}
    CorryvreckanWriterModule.cpp
)


# Include Geant4 directories (NOTE Geant4_USE_FILE is not used!)
TARGET_INCLUDE_DIRECTORIES(${MODULE_NAME} SYSTEM PRIVATE ${Geant4_INCLUDE_DIRS})

TARGET_LINK_LIBRARIES(${MODULE_NAME} CorryvreckanObjects ROOT::Tree ${Geant4_LIBRARIES})

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
