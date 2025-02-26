# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Include the dependencies
INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})

# Create core library
ADD_LIBRARY(
    AllpixCore SHARED
    utils/log.cpp
    utils/text.cpp
    utils/unit.cpp
    module/Module.cpp
    module/Event.cpp
    module/ModuleManager.cpp
    module/ThreadPool.cpp
    messenger/Messenger.cpp
    messenger/Message.cpp
    config/exceptions.cpp
    config/Configuration.cpp
    config/ConfigReader.cpp
    config/ConfigManager.cpp
    config/OptionParser.cpp
    geometry/Detector.cpp
    geometry/DetectorModel.cpp
    geometry/PixelDetectorModel.cpp
    geometry/GeometryManager.cpp
    geometry/HexagonalPixelDetectorModel.cpp
    geometry/RadialStripDetectorModel.cpp
    Allpix.cpp)

# Link the dependencies
TARGET_LINK_LIBRARIES(AllpixCore PUBLIC ${ALLPIX_DEPS_LIBRARIES})
TARGET_LINK_LIBRARIES(AllpixCore PRIVATE ${ALLPIX_LIBRARIES})

# Define compile-time library extension
TARGET_COMPILE_DEFINITIONS(AllpixCore PRIVATE SHARED_LIBRARY_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}")
# Link the DL libraries
TARGET_LINK_LIBRARIES(AllpixCore PRIVATE ${CMAKE_DL_LIBS})
TARGET_INCLUDE_DIRECTORIES(AllpixCore PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include>)

# Create standard install target
INSTALL(
    TARGETS AllpixCore
    EXPORT Allpix
    COMPONENT application
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)

INSTALL(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DESTINATION include
    FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "*.h"
    PATTERN "*.tpp"
    PATTERN "dynamic_module_impl.cpp"
    PATTERN "Allpix.hpp" EXCLUDE)
