cmake_minimum_required(VERSION 3.8)

# Include the standard dependencies
INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})

ADD_EXECUTABLE(mesh_plot mapping.cpp)

TARGET_LINK_LIBRARIES(mesh_plot ${ALLPIX_DEPS_LIBRARIES})
# Link the dependency libraries

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
