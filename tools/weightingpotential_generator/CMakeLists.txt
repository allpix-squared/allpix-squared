# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# CMake file for the Weighting Potential Generator of the Allpix Squared framework
CMAKE_MINIMUM_REQUIRED(VERSION 3.6.3 FATAL_ERROR)
IF(COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0003 NEW) # change linker path search behaviour
    CMAKE_POLICY(SET CMP0048 NEW) # set project version
ENDIF(COMMAND CMAKE_POLICY)

# Check if a version number is set - if not, just default to an empty string
IF(NOT ALLPIX_VERSION)
    ADD_DEFINITIONS(-DALLPIX_PROJECT_VERSION="")
ENDIF()

# ROOT is required for vector and persistency etc
FIND_PACKAGE(ROOT REQUIRED NO_MODULE)
IF(NOT ROOT_FOUND)
    MESSAGE(FATAL_ERROR "Could not find ROOT, make sure to source the ROOT environment\n"
                        "$ source YOUR_ROOT_DIR/bin/thisroot.sh")
ENDIF()
ALLPIX_SETUP_ROOT_TARGETS()

# Find Threading library
FIND_PACKAGE(Threads REQUIRED)

# Find required Allpix Squared tools
GET_FILENAME_COMPONENT(ALLPIX_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../../src/" ABSOLUTE)
INCLUDE_DIRECTORIES(${ALLPIX_SRC})

# Add TCAD dfise converter executable
ADD_EXECUTABLE(
    generate_potential
    WeightingPotentialGenerator.cpp
    ${ALLPIX_SRC}/core/utils/log.cpp
    ${ALLPIX_SRC}/core/utils/text.cpp
    ${ALLPIX_SRC}/core/utils/unit.cpp
    ${ALLPIX_SRC}/core/config/ConfigReader.cpp
    ${ALLPIX_SRC}/core/config/Configuration.cpp
    ${ALLPIX_SRC}/core/config/exceptions.cpp
    ${ALLPIX_SRC}/core/geometry/DetectorModel.cpp
    ${ALLPIX_SRC}/core/geometry/PixelDetectorModel.cpp
    ${ALLPIX_SRC}/core/geometry/BrickwallPixelDetectorModel.cpp
    ${ALLPIX_SRC}/core/geometry/HexagonalPixelDetectorModel.cpp
    ${ALLPIX_SRC}/core/geometry/RadialStripDetectorModel.cpp
    ${ALLPIX_SRC}/core/module/ThreadPool.cpp)

# Include Eigen dependency
FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(Eigen3 REQUIRED IMPORTED_TARGET eigen3)

# Link the dependency libraries
TARGET_LINK_LIBRARIES(generate_potential ROOT::Core Threads::Threads PkgConfig::Eigen3)

# Create install target
INSTALL(
    TARGETS generate_potential
    COMPONENT tools
    RUNTIME DESTINATION bin)
