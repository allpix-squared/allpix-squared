# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Option to build all modules
OPTION(BUILD_ALL_MODULES "Build all modules?" OFF)

# reset the saved libraries
SET(_ALLPIX_MODULE_LIBRARIES
    ""
    CACHE INTERNAL "Module libraries")

# Generate an interface library containing all modules:
ADD_LIBRARY(Modules INTERFACE)

# Build all the modules
FILE(
    GLOB subdirs
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/*)
FOREACH(subdir ${subdirs})
    IF(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
        # Internal macros check if the module is actually enabled
        ADD_SUBDIRECTORY(${subdir})
    ENDIF()
ENDFOREACH()

INSTALL(
    TARGETS Modules
    EXPORT Allpix
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
