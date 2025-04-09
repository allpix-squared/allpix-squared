# SPDX-FileCopyrightText: 2016-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

###############################################
# CMake File for the Allpix Squared Framework #
###############################################

CMAKE_MINIMUM_REQUIRED(VERSION 3.7.0 FATAL_ERROR)
IF(COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0003 NEW) # change linker path search behaviour
    CMAKE_POLICY(SET CMP0048 NEW) # set project version
    IF(${CMAKE_VERSION} VERSION_GREATER "3.13")
        CMAKE_POLICY(SET CMP0077 NEW) # allow overwriting options with normal variables
        CMAKE_POLICY(SET CMP0079 NEW) # Allow lookup of linking libraries in other directories
    ENDIF()
ENDIF(COMMAND CMAKE_POLICY)

# Set default version
SET(ALLPIX_VERSION "v3.1.3")

# Set default build type
IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE
        "RelWithDebInfo"
        CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# Overwrite with the version from git if found
INCLUDE("cmake/tools.cmake")
GET_VERSION(ALLPIX_VERSION)

# Print version
MESSAGE(STATUS "Building Allpix^2 version ${ALLPIX_VERSION}.")
MESSAGE(STATUS "Building \"${CMAKE_BUILD_TYPE}\"")

# Gather information about build time:
STRING(TIMESTAMP BUILD_TIME "%Y-%m-%d, %H:%M:%S UTC" UTC)
STRING(SUBSTRING "${BUILD_TIME}" 0 10 BUILD_DATE)

# Define the project with the simple version
STRING(REGEX MATCH "([0-9.]+)+" SIMPLE_VERSION "${ALLPIX_VERSION}")
# Set languages to NONE to allow the documentation to be built without CXX compiler:
PROJECT(
    Allpix
    VERSION ${SIMPLE_VERSION}
    LANGUAGES NONE)
# Access the project name (for install locations) in the source
ADD_DEFINITIONS(-DALLPIX_PROJECT_NAME="${CMAKE_PROJECT_NAME}" -DALLPIX_PROJECT_VERSION="${ALLPIX_VERSION}"
                -DALLPIX_BUILD_TIME="${BUILD_TIME}")

# Read possible build environment
IF(DEFINED ENV{ALLPIX_BUILD_ENV})
    MESSAGE(STATUS "Building in environment \"$ENV{ALLPIX_BUILD_ENV}\"")
    ADD_DEFINITIONS(-DALLPIX_BUILD_ENV=\"$ENV{ALLPIX_BUILD_ENV}\")
ENDIF()

# Include a generated configuration file
# FIXME: this should be combined with the ADD_DEFINITIONS
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.h" "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

# Configure the installation prefix to allow both local as system-wide installation
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CMAKE_INSTALL_PREFIX
        "${PROJECT_SOURCE_DIR}"
        CACHE PATH "Prefix prepended to install directories" FORCE)
ENDIF()
MESSAGE(STATUS "Installing to \"${CMAKE_INSTALL_PREFIX}\"")

INCLUDE(cmake/CPackConfig.cmake)
INCLUDE(CPack)

# Include allpix cmake functions
INCLUDE("cmake/AllpixMacros.cmake")

# Add cmake/Modules to CMake search path
LIST(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules")

###################################
# Setup tests for allpix          #
###################################

OPTION(TEST_CORE "Perform unit tests to ensure framework core functionality?" ON)
OPTION(TEST_MODULES "Perform unit tests to ensure module functionality?" ON)
OPTION(TEST_PERFORMANCE "Perform unit tests to ensure framework performance?" ON)
OPTION(TEST_EXAMPLES "Perform unit tests to ensure example validity?" ON)

SET(_MODULES_WITH_TESTS
    ""
    CACHE INTERNAL "MODULES_WITH_TESTS")

# Enable testing
ENABLE_TESTING()

# Include example configurations:
ADD_SUBDIRECTORY(examples)

# Include all tests
ADD_SUBDIRECTORY(etc/unittests)

###############################################
# Setup the environment for the documentation #
###############################################

OPTION(BUILD_DOCS_ONLY "Build documentation only" OFF)

# Variables to configure list of tests
SET(DOCS_TESTS_UNCONF "${CMAKE_CURRENT_SOURCE_DIR}/doc/usermanual/15_appendix/tests.md.in")
SET(DOCS_TESTS_CONF "${CMAKE_CURRENT_BINARY_DIR}/doc/tests.md")

# Add targets for Doxygen code reference and user manual
ADD_SUBDIRECTORY(doc)

# If only building docs, stop processing the rest of the CMake file:
IF(BUILD_DOCS_ONLY)
    FILE(
        GLOB_RECURSE module_tests
        LIST_DIRECTORIES false
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/src/modules"
        ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/*/*/[00-99]*.conf)
    FOREACH(test ${module_tests})
        GET_FILENAME_COMPONENT(title ${test} NAME_WE)
        GET_FILENAME_COMPONENT(dir "${test}/../.." ABSOLUTE)
        GET_FILENAME_COMPONENT(mod "${dir}" NAME)
        ADD_ALLPIX_TEST(NAME "modules/${mod}/${title}" FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/${test})
    ENDFOREACH()

    SET(MODULES_TEST_DESCRIPTIONS "${TEST_DESCRIPTIONS}")
    GET_PROPERTY(CORE_TEST_DESCRIPTIONS GLOBAL PROPERTY CORE_TEST_DESCRIPTIONS)
    GET_PROPERTY(PERF_TEST_DESCRIPTIONS GLOBAL PROPERTY PERF_TEST_DESCRIPTIONS)
    STRING(REPLACE ";" " " CORE_TEST_DESCRIPTIONS "${CORE_TEST_DESCRIPTIONS}")
    STRING(REPLACE ";" " " MODULES_TEST_DESCRIPTIONS "${MODULES_TEST_DESCRIPTIONS}")
    STRING(REPLACE ";" " " PERF_TEST_DESCRIPTIONS "${PERF_TEST_DESCRIPTIONS}")
    CONFIGURE_FILE("${DOCS_TESTS_UNCONF}" "${DOCS_TESTS_CONF}" @ONLY)
    RETURN()
ENDIF()

###############################
# Setup the build environment #
###############################

# Enable CXX as project language to perform compiler checks:
ENABLE_LANGUAGE(CXX)

# Fix for CMake when Geant4 has HDF5 / VTK enabled
ENABLE_LANGUAGE(C)

# Additional packages to be searched for by cmake
LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Set up the RPATH so executables find the libraries even when installed in non-default location
SET(CMAKE_MACOSX_RPATH TRUE)
SET(CMAKE_SKIP_BUILD_RPATH FALSE)
SET(CMAKE_SKIP_INSTALL_RPATH FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
SET(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})

# Add the automatically determined parts of the RPATH which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# The RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" IsSystemDir)
IF("${IsSystemDir}" STREQUAL "-1")
    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${IsSystemDir}" STREQUAL "-1")

IF(APPLE)
    SET(CMAKE_INSTALL_NAME_DIR "@rpath")
    SET(CMAKE_INSTALL_RPATH "@loader_path/../lib") # self relative LIBDIR
ENDIF()

# If available, use CMake's link-what-you-use feature, not on APPLE
IF(NOT APPLE AND NOT CMAKE_LINK_WHAT_YOU_USE)
    SET(CMAKE_LINK_WHAT_YOU_USE
        TRUE
        CACHE STRING "Choose whether to only link libraries which contain symbols actually used by the target." FORCE)
ENDIF()

# We need PkgConfig for some dependencies:
FIND_PACKAGE(PkgConfig REQUIRED)


#################################
# Figure out ROOT's C++ version #
#################################

FIND_PACKAGE(ROOT REQUIRED COMPONENTS Geom Core GenVector Hist RIO NO_MODULE)
IF(NOT ROOT_FOUND)
    MESSAGE(FATAL_ERROR "Could not find ROOT, make sure to source the ROOT environment\n"
                        "$ source YOUR_ROOT_DIR/bin/thisroot.sh")
ENDIF()

# Check which C++ version ROOT was built against
IF(ROOT_CXX_STANDARD MATCHES "20")
    SET(ROOT_CXX_STD 20)
ELSEIF(ROOT_CXX_STANDARD MATCHES "17")
    SET(ROOT_CXX_STD 17)
ELSEIF(NOT ROOT_CXX_STANDARD)
    #ROOT_CXX_STANDARD does not exist for ROOT versions earlier than 6.30.07.
    MESSAGE(WARNING "Could not find ROOT_CXX_STANDARD environment variable. Attempt to deduce from ROOT_CXX_FLAGS")
    IF(ROOT_CXX_FLAGS MATCHES ".*std=c\\+\\+2[0a].*")
        SET(ROOT_CXX_STD 20)
    ELSEIF(ROOT_CXX_FLAGS MATCHES ".*std=c\\+\\+1[7z].*")
        SET(ROOT_CXX_STD 17)
    ELSEIF(ROOT_CXX_FLAGS MATCHES ".*std=c\\+\\+.*")
        MESSAGE(FATAL_ERROR "ROOT was built with an unsupported C++ version, at least C++17 is required: ${ROOT_CXX_FLAGS}")
    ELSE()
        MESSAGE(FATAL_ERROR "Could not deduce ROOT's C++ version from build flags: ${ROOT_CXX_FLAGS}")
    ENDIF()
ELSE()
    MESSAGE(FATAL_ERROR "ROOT was built with an unsupported C++ version, at least C++17 is required: ${ROOT_CXX_STANDARD}")
ENDIF()

# Check ROOT version
IF(NOT ${ROOT_VERSION} VERSION_GREATER "6.0")
    MESSAGE(FATAL_ERROR "ROOT versions below 6.0 are not supported")
ENDIF()


###################################
# Define build flags for allpix   #
###################################

# Set standard build flags
SET(COMPILER_FLAGS
    -pedantic
    -Wall
    -Wextra
    -Wcast-align
    -Wcast-qual
    -Wconversion
    -Wuseless-cast
    -Wctor-dtor-privacy
    -Wzero-as-null-pointer-constant
    -Wdisabled-optimization
    -Wformat=2
    -Winit-self
    -Wlogical-op
    -Wmissing-declarations
    -Wmissing-include-dirs
    -Wnoexcept
    -Wold-style-cast
    -Wredundant-decls
    -Wsign-conversion
    -Wsign-promo
    -Wstrict-null-sentinel
    -Wstrict-overflow=5
    -Wswitch-default
    -Wundef
    -Wshadow
    -Wformat-security
    -Wdeprecated
    -fdiagnostics-color=auto
    -Wheader-hygiene
    -Wno-overloaded-virtual)

INCLUDE("cmake/compiler-flag-checks.cmake")

# Use CCache if available
FIND_PROGRAM(CCACHE_FOUND ccache)
IF(CCACHE_FOUND)
    MESSAGE(STATUS "Using Ccache: ${CCACHE_FOUND}")
    SET(CMAKE_CXX_COMPILER_LAUNCHER
        "${CCACHE_FOUND}"
        CACHE PATH "CCache program" FORCE)
ENDIF()

# Require C++17 or C++20
CHECK_CXX_COMPILER_FLAG(-std=c++17 SUPPORT_STD_CXX17)
CHECK_CXX_COMPILER_FLAG(-std=c++20 SUPPORT_STD_CXX20)
IF(ROOT_CXX_STD EQUAL 20 AND NOT SUPPORT_STD_CXX20)
    MESSAGE(FATAL_ERROR "Compiler does not support standard required by ROOT: C++20")
ELSEIF(ROOT_CXX_STD EQUAL 17 AND NOT SUPPORT_STD_CXX17)
    MESSAGE(FATAL_ERROR "Compiler does not support standard required by ROOT: C++17")
ENDIF()

# Build with C++20 or C++17
SET(CMAKE_CXX_STANDARD "${ROOT_CXX_STD}")
MESSAGE(STATUS "Building against C++ version ${CMAKE_CXX_STANDARD}")
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

# Check if compiler version supports all features:
INCLUDE("cmake/compiler-version-checks.cmake")

# Options for debug builds:
OPTION(SANITIZER "Build with sanitizer flags" OFF)
OPTION(COVERAGE "Create code coverage report" OFF)

# FIXME: not using the flag checker now because it wrongly rejects a sanitizer flag..
IF(CMAKE_BUILD_TYPE MATCHES Debug AND ((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")))
    # Sanitizer libraries:
    IF(SANITIZER)
        MESSAGE(STATUS "Running debug build, adding extra sanitizer flags")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")
        # FIXME: remove earlier shared linker flags, because they can break the sanitizer debug build
        SET(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=address -fsanitize=undefined")
    ENDIF()

    # Create code coverage report for central library
    IF(COVERAGE)
        INCLUDE("cmake/CodeCoverage.cmake")
        APPEND_COVERAGE_COMPILER_FLAGS()
        SET(COVERAGE_GCOVR_EXCLUDES "${PROJECT_SOURCE_DIR}/src/modules" "${PROJECT_SOURCE_DIR}/tools"
                                    "${PROJECT_SOURCE_DIR}/3rdparty" "${PROJECT_BINARY_DIR}")
        SETUP_TARGET_FOR_COVERAGE_GCOVR(
            NAME coverage_gcovr
            EXECUTABLE ctest -L coverage
            DEPENDENCIES allpix AllpixCore)
        SETUP_TARGET_FOR_COVERAGE_GCOVR_HTML(
            NAME coverage_gcovr_html
            EXECUTABLE ctest -L coverage
            DEPENDENCIES allpix AllpixCore)
    ENDIF()
ENDIF()

# Include Threads
FIND_PACKAGE(Threads REQUIRED)

# Include Boost.Random
FIND_PACKAGE(Boost 1.64.0 REQUIRED COMPONENTS random)

# Include Eigen3 without requiring it at this moment
PKG_CHECK_MODULES(Eigen3 QUIET IMPORTED_TARGET eigen3)

#####################################
# Prerequisistes for Allpix Squared #
#####################################

# Define the libraries
SET(ALLPIX_LIBRARIES "")

# Define the optional libraries
SET(ALLPIX_GEANT4_INTERFACE "AllpixGeant4Interface")
SET(ALLPIX_BUILD_GEANT4_INTERFACE
    "OFF"
    CACHE BOOL "Build Geant4 interface library" FORCE)

# Prepare ROOT Targets if necessary:
ALLPIX_SETUP_ROOT_TARGETS()

# Set the dependencies
SET(ALLPIX_DEPS_INCLUDE_DIRS ${ROOT_INCLUDE_DIRS})
SET(ALLPIX_DEPS_LIBRARIES
    Threads::Threads
    Boost::random
    ROOT::Core
    ROOT::GenVector
    ROOT::Geom
    ROOT::RIO
    ROOT::Hist)

# If Eigen3 has been found, link it:
IF(Eigen3_FOUND)
    LIST(APPEND ALLPIX_DEPS_LIBRARIES PkgConfig::Eigen3)
ENDIF()

# Add the LCG view as dependency if set:
IF(DEFINED ENV{LCG_VIEW})
    ADD_RUNTIME_DEP($ENV{LCG_VIEW})
ENDIF()

# Add "thisroot.sh" as runtime dependency for setup.sh file:
ADD_RUNTIME_DEP(thisroot.sh)

###################################
# Load cpp format and check tools #
###################################

# Set the clang-format version required by the CI for correct formatting:
SET(CLANG_FORMAT_VERSION "16")
# Set the clang-tidy version of the linter required by the CI:
SET(CLANG_TIDY_VERSION "16")

# Set the source files to clang-format (FIXME: determine this better)
FILE(
    GLOB_RECURSE
    CHECK_CXX_SOURCE_FILES
    src/*.[tch]pp
    src/*.h
    tools/*.C
    tools/*.[tch]pp
    tools/*.h)

INCLUDE("cmake/clang-cpp-checks.cmake")

FILE(
    GLOB_RECURSE CHECK_CMAKE_FILES
    LIST_DIRECTORIES false
    RELATIVE ${PROJECT_SOURCE_DIR}
    "cmake/*.cmake" "CMakeLists.txt")
LIST(REMOVE_ITEM CHECK_CMAKE_FILES "cmake/CodeCoverage.cmake")

INCLUDE("cmake/cmake-checks.cmake")

###################################
# Define build targets for allpix #
###################################

# Set the models directory and provide install if necessary
ADD_SUBDIRECTORY(models)
ADD_DEFINITIONS(-DALLPIX_MODEL_DIRECTORY="${MODEL_DIRECTORY}" -DALLPIX_MODEL_SUFFIX="${_MODEL_SUFFIX}")

# Include third-party libraries as system headers:
INCLUDE_DIRECTORIES(SYSTEM 3rdparty)

# Always include sources from top directory
INCLUDE_DIRECTORIES(src)

# Build objects library
ADD_SUBDIRECTORY(src/objects)
SET(ALLPIX_LIBRARIES ${ALLPIX_LIBRARIES} AllpixObjects)

# Build core allpix library
ADD_SUBDIRECTORY(src/core)
SET(ALLPIX_LIBRARIES ${ALLPIX_LIBRARIES} AllpixCore)

# Build required modules
ADD_SUBDIRECTORY(src/modules)

# Build geant4 interface library if needed by modules
IF(${ALLPIX_BUILD_GEANT4_INTERFACE})
    ADD_SUBDIRECTORY(src/tools/geant4)
    ADD_DEFINITIONS(-DALLPIX_GEANT4_AVAILABLE)
ENDIF()

# Build the executable
ADD_SUBDIRECTORY(src/exec)

# Handle the included tools
ADD_SUBDIRECTORY(tools)

##################
# Test summaries #
##################

IF(TEST_CORE)
    GET_PROPERTY(CNT GLOBAL PROPERTY COUNT_TESTS_CORE)
    MESSAGE(STATUS "Unit tests: ${CNT} framework core functionality tests")
ELSE()
    MESSAGE(STATUS "Unit tests: framework core functionality tests deactivated.")
ENDIF()

IF(TEST_MODULES)
    GET_PROPERTY(CNT GLOBAL PROPERTY COUNT_TESTS_MODULES)
    MESSAGE(STATUS "Unit tests: ${CNT} module functionality tests")

    SET(ALLPIX_MODULE_DIR "${PROJECT_SOURCE_DIR}/src/modules")
    MESSAGE(STATUS "Modules without functionality tests:")
    FILE(
        GLOB mods
        RELATIVE ${ALLPIX_MODULE_DIR}/
        ${ALLPIX_MODULE_DIR}/*)
    FOREACH(mod ${mods})
        # Check if module has registered tests and is going to be built
        IF(IS_DIRECTORY ${ALLPIX_MODULE_DIR}/${mod}
           AND NOT "${mod}" IN_LIST _MODULES_WITH_TESTS
           AND "${BUILD_${mod}}")
            MESSAGE(STATUS "   ${mod}")
        ENDIF()
    ENDFOREACH()
ELSE()
    MESSAGE(STATUS "Unit tests: module functionality tests deactivated.")
ENDIF()

IF(TEST_PERFORMANCE)
    GET_PROPERTY(CNT GLOBAL PROPERTY COUNT_TESTS_PERFORMANCE)
    MESSAGE(STATUS "Unit tests: ${CNT} performance tests")
ELSE()
    MESSAGE(STATUS "Unit tests: performance tests deactivated.")
ENDIF()

GET_PROPERTY(CORE_TEST_DESCRIPTIONS GLOBAL PROPERTY CORE_TEST_DESCRIPTIONS)
GET_PROPERTY(MODULES_TEST_DESCRIPTIONS GLOBAL PROPERTY MODULES_TEST_DESCRIPTIONS)
GET_PROPERTY(PERF_TEST_DESCRIPTIONS GLOBAL PROPERTY PERF_TEST_DESCRIPTIONS)
STRING(REPLACE ";" " " CORE_TEST_DESCRIPTIONS "${CORE_TEST_DESCRIPTIONS}")
STRING(REPLACE ";" " " MODULES_TEST_DESCRIPTIONS "${MODULES_TEST_DESCRIPTIONS}")
STRING(REPLACE ";" " " PERF_TEST_DESCRIPTIONS "${PERF_TEST_DESCRIPTIONS}")
CONFIGURE_FILE("${DOCS_TESTS_UNCONF}" "${DOCS_TESTS_CONF}" @ONLY)

############################
# Create local setup files #
############################

# Add compiler if necessary:
GET_FILENAME_COMPONENT(CMP_PATH ${CMAKE_CXX_COMPILER} DIRECTORY)
GET_FILENAME_COMPONENT(PARENT_PATH ${CMP_PATH} DIRECTORY)
IF(EXISTS ${PARENT_PATH}/setup.sh)
    LIST(APPEND _ALLPIX_RUNTIME_DEPS "${PARENT_PATH}/setup.sh")
ENDIF()

# Build configuration string with commands
FOREACH(dep ${_ALLPIX_RUNTIME_DEPS})
    GET_FILENAME_COMPONENT(DEP_PATH ${dep} DIRECTORY)
    SET(SETUP_FILE_DEPS "${SETUP_FILE_DEPS}cd ${DEP_PATH}\n")
    SET(SETUP_FILE_DEPS "${SETUP_FILE_DEPS}source ${dep}\n")
    SET(SETUP_FILE_DEPS "${SETUP_FILE_DEPS}cd -\n\n")
ENDFOREACH()
FOREACH(dep ${_ALLPIX_RUNTIME_LIBS})
    SET(SETUP_FILE_DEPS "${SETUP_FILE_DEPS}export LD_LIBRARY_PATH=\"${dep}:$LD_LIBRARY_PATH\"\n")
ENDFOREACH()
# Create setup file
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/cmake/setup.cmake.sh" "${CMAKE_CURRENT_BINARY_DIR}/setup/setup.sh" @ONLY)

#######################################
# Generate CMake Configuration Module #
#######################################

INCLUDE(CMakePackageConfigHelpers)
SET(ALLPIX_INCLUDE_DIR "include")
SET(ALLPIX_LIBRARY_DIR "lib")
SET(ALLPIX_CMAKE_DIR "share/cmake")
CONFIGURE_PACKAGE_CONFIG_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AllpixConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/AllpixConfig.cmake
    INSTALL_DESTINATION ${ALLPIX_CMAKE_DIR}
    PATH_VARS ALLPIX_INCLUDE_DIR ALLPIX_LIBRARY_DIR)

WRITE_BASIC_PACKAGE_VERSION_FILE(
    AllpixConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)
INSTALL(FILES ${PROJECT_BINARY_DIR}/AllpixConfig.cmake ${PROJECT_BINARY_DIR}/AllpixConfigVersion.cmake
              ${PROJECT_SOURCE_DIR}/cmake/AllpixMacros.cmake DESTINATION ${ALLPIX_CMAKE_DIR})

INSTALL(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/tools"
    DESTINATION include
    PATTERN "*.h")

INSTALL(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/magic_enum"
    DESTINATION include/3rdparty
    PATTERN "*.hpp")

INSTALL(
    EXPORT Allpix
    NAMESPACE Allpix::
    FILE AllpixConfigTargets.cmake
    DESTINATION ${ALLPIX_CMAKE_DIR})
