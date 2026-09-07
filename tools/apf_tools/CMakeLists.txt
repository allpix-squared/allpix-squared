# SPDX-FileCopyrightText: 2019-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# CMake file for the allpix2 framework
CMAKE_MINIMUM_REQUIRED(VERSION 3.22.1 FATAL_ERROR)
IF(COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0003 NEW) # change linker path search behaviour
    CMAKE_POLICY(SET CMP0048 NEW) # set project version
ENDIF(COMMAND CMAKE_POLICY)

# Check if a version number is set - if not, just default to an empty string
IF(NOT ALLPIX_VERSION)
    ADD_DEFINITIONS(-DALLPIX_PROJECT_VERSION="")
ENDIF()

# Logging backend
FIND_PACKAGE(spdlog REQUIRED)

# Find required Allpix Squared tools
GET_FILENAME_COMPONENT(ALLPIX_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../../src/" ABSOLUTE)
INCLUDE_DIRECTORIES(${ALLPIX_SRC})
INCLUDE_DIRECTORIES(${ALLPIX_SRC}/tools)

SET(ALLPIX_LOG_SRCS
    ${ALLPIX_SRC}/core/log/LogContext.cpp
    ${ALLPIX_SRC}/core/log/Logger.cpp
    ${ALLPIX_SRC}/core/log/LoggerManager.cpp
    ${ALLPIX_SRC}/core/log/Sinks.cpp)

# Small field converter tool
ADD_EXECUTABLE(field_converter FieldConverter.cpp ${ALLPIX_LOG_SRCS} ${ALLPIX_SRC}/core/utils/text.cpp
                               ${ALLPIX_SRC}/core/utils/unit.cpp)

# Set compiler options
TARGET_COMPILE_OPTIONS(field_converter PRIVATE ${ALLPIX_CXX_FLAGS})
TARGET_LINK_LIBRARIES(field_converter spdlog::spdlog)

# Create install target
INSTALL(
    TARGETS field_converter
    COMPONENT tools
    RUNTIME DESTINATION bin)

# Tiny header dump tool for APF
ADD_EXECUTABLE(apf_dump DumpHeader.cpp ${ALLPIX_LOG_SRCS} ${ALLPIX_SRC}/core/utils/text.cpp
                        ${ALLPIX_SRC}/core/utils/unit.cpp)

# Set compiler options
TARGET_COMPILE_OPTIONS(apf_dump PRIVATE ${ALLPIX_CXX_FLAGS})
TARGET_LINK_LIBRARIES(apf_dump spdlog::spdlog)

# Create install target
INSTALL(
    TARGETS apf_dump
    COMPONENT tools
    RUNTIME DESTINATION bin)
