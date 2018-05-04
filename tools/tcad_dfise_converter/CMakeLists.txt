# CMake file for the allpix2 framework
CMAKE_MINIMUM_REQUIRED(VERSION 3.4.3 FATAL_ERROR)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW) # change linker path search behaviour
  CMAKE_POLICY(SET CMP0048 NEW) # set project version
ENDIF(COMMAND CMAKE_POLICY)

# ROOT is required for vector and persistency etc
FIND_PACKAGE(ROOT REQUIRED)
IF(NOT ROOT_FOUND)
    MESSAGE(FATAL_ERROR "Could not find ROOT, make sure to source the ROOT environment\n"
    "$ source YOUR_ROOT_DIR/bin/thisroot.sh")
ENDIF()
INCLUDE_DIRECTORIES(SYSTEM ${ROOT_INCLUDE_DIRS})

# Find required Allpix Squared tools
GET_FILENAME_COMPONENT(ALLPIX_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../../src/" ABSOLUTE)
INCLUDE_DIRECTORIES(${ALLPIX_SRC})

# Add TCAD dfise converter executable
ADD_EXECUTABLE(dfise_converter
    MeshElement.cpp
    dfise_converter.cpp
    read_dfise.cpp
    ${ALLPIX_SRC}/core/utils/log.cpp
    ${ALLPIX_SRC}/core/utils/text.cpp
    ${ALLPIX_SRC}/core/utils/unit.cpp
    ${ALLPIX_SRC}/core/config/ConfigReader.cpp
    ${ALLPIX_SRC}/core/config/Configuration.cpp
    ${ALLPIX_SRC}/core/config/exceptions.cpp
)

#Build the TCAD converter
ADD_SUBDIRECTORY(data)

# Include Eigen dependency
FIND_PACKAGE(Eigen3 REQUIRED)

# Link the dependency libraries
TARGET_LINK_LIBRARIES(dfise_converter ${ROOT_LIBRARIES} ${ROOT_COMPONENT_LIBRARIES} Eigen3::Eigen)

# Create install target
INSTALL(TARGETS dfise_converter
    COMPONENT tools
    RUNTIME DESTINATION bin/tcad_dfise_converter)
