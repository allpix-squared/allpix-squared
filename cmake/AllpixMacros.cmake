# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# For every module, build a separate library to be loaded by allpix core
MACRO(ALLPIX_ENABLE_DEFAULT val)
    # Get the name of the module
    GET_FILENAME_COMPONENT(_allpix_module_dir ${CMAKE_CURRENT_SOURCE_DIR} NAME)

    # Build all modules by default if not specified otherwise
    OPTION(BUILD_${_allpix_module_dir} "Build module in directory ${_allpix_module_dir}?" ${val})
ENDMACRO()

# Common module definitions
MACRO(_allpix_module_define_common name)
    # Get the name of the module
    GET_FILENAME_COMPONENT(_allpix_module_dir ${CMAKE_CURRENT_SOURCE_DIR} NAME)

    # Build all modules by default if not specified otherwise
    OPTION(BUILD_${_allpix_module_dir} "Build module in directory ${_allpix_module_dir}?" ON)

    # Put message
    MESSAGE(STATUS "Building module " ${BUILD_${_allpix_module_dir}} "\t- " ${_allpix_module_dir})

    # Quit the file if not building this file or all modules
    IF(NOT (BUILD_${_allpix_module_dir} OR BUILD_ALL_MODULES))
        RETURN()
    ENDIF()

    # Prepend with the allpix module prefix to create the name of the module
    SET(${name} "AllpixModule${_allpix_module_dir}")

    # Save the module library for prelinking in the executable (NOTE: see exec folder)
    SET(_ALLPIX_MODULE_LIBRARIES
        ${_ALLPIX_MODULE_LIBRARIES} ${${name}}
        CACHE INTERNAL "Module libraries")

    # Set default module class name
    SET(_allpix_module_class "${_allpix_module_dir}Module")

    # Find if alternative module class name is passed or we can use the default
    SET(extra_macro_args ${ARGN})
    LIST(LENGTH extra_macro_args num_extra_args)
    IF(${num_extra_args} GREATER 0)
        MESSAGE(AUTHOR_WARNING "Provided non-standard module class name! Naming it ${_allpix_module_class} is recommended")
        LIST(GET extra_macro_args 0 _allpix_module_class)
    ENDIF()

    # check if main header file is defined
    IF(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_allpix_module_class}.hpp")
        MESSAGE(FATAL_ERROR "Header file ${_allpix_module_class}.hpp does not exist, cannot build module! \
Create the header or provide the alternative class name as first argument")
    ENDIF()

    # Define the library
    ADD_LIBRARY(${${name}} SHARED "")

    # Set compiler options
    TARGET_COMPILE_OPTIONS(${${name}} PRIVATE ${ALLPIX_CXX_FLAGS})

    # Add the current directory as include directory
    TARGET_INCLUDE_DIRECTORIES(${${name}} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    # Set the special header flags and add the special dynamic implementation file
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE ALLPIX_MODULE_NAME=${_allpix_module_class})
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE ALLPIX_MODULE_HEADER="${_allpix_module_class}.hpp")

    # If modules are build externally, the path to the dynamic implementation changes and we need to link differently:
    IF(${ALLPIX_MODULE_EXTERNAL})
        # Add 3rdparty includes
        TARGET_INCLUDE_DIRECTORIES(${${name}} SYSTEM PRIVATE ${ALLPIX_INCLUDE_DIR}/3rdparty)

        TARGET_SOURCES(${${name}} PRIVATE "${ALLPIX_INCLUDE_DIR}/core/module/dynamic_module_impl.cpp")
        SET_PROPERTY(
            SOURCE "${ALLPIX_INCLUDE_DIR}/dynamic_module_impl.cpp"
            APPEND
            PROPERTY OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_allpix_module_class}.hpp")
    ELSE()
        TARGET_SOURCES(${${name}} PRIVATE "${PROJECT_SOURCE_DIR}/src/core/module/dynamic_module_impl.cpp")
        SET_PROPERTY(
            SOURCE "${PROJECT_SOURCE_DIR}/src/core/module/dynamic_module_impl.cpp"
            APPEND
            PROPERTY OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_allpix_module_class}.hpp")
        SET_PROPERTY(
            SOURCE "${PROJECT_SOURCE_DIR}/src/core/module/Module.cpp"
            APPEND
            PROPERTY OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_allpix_module_class}.hpp")

        # Add to the interface library for devices:
        TARGET_LINK_LIBRARIES(Modules INTERFACE ${${name}})
    ENDIF()
ENDMACRO()

# Put this at the start of every unique module
MACRO(ALLPIX_UNIQUE_MODULE name)
    _ALLPIX_MODULE_DEFINE_COMMON(${name} ${ARGN})

    # Set the unique flag to true
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE ALLPIX_MODULE_UNIQUE=1)
ENDMACRO()

# Put this at the start of every detector module
MACRO(ALLPIX_DETECTOR_MODULE name)
    _ALLPIX_MODULE_DEFINE_COMMON(${name} ${ARGN})

    # Set the unique flag to false
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE ALLPIX_MODULE_UNIQUE=0)
ENDMACRO()

# Add sources to the module
MACRO(ALLPIX_MODULE_SOURCES name)
    # Get the list of sources
    SET(_list_var "${ARGN}")
    LIST(REMOVE_ITEM _list_var ${name})

    # Include directories for dependencies
    INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})

    # Add the library
    TARGET_SOURCES(${name} PRIVATE ${_list_var})

    # Link the standard allpix libraries, either directly or via targets
    IF(${ALLPIX_MODULE_EXTERNAL})
        TARGET_LINK_LIBRARIES(${name} Allpix::AllpixCore Allpix::AllpixObjects)
    ELSE()
        TARGET_LINK_LIBRARIES(${name} ${ALLPIX_LIBRARIES} ${ALLPIX_DEPS_LIBRARIES})
    ENDIF()
ENDMACRO()

# Escape regular expressions in the match strings of tests
FUNCTION(escape_regex inp output)
    # Escape possible regex patterns in the expected output:
    STRING(REPLACE "#PASS " "" _TMP_STR "${inp}")
    STRING(REPLACE "#FAIL " "" _TMP_STR "${_TMP_STR}")
    STRING(REGEX REPLACE "([][+.*()^])" "\\\\\\1" _TMP_STR "${_TMP_STR}")
    STRING(REGEX REPLACE "(\\\\n)" "[\\\\\\\\\r\\\\\\\\\n\\\\\\\\\t ]*" _TMP_STR "${_TMP_STR}")
    SET(${output}
        "${_TMP_STR}"
        PARENT_SCOPE)
ENDFUNCTION()

# Add default fail conditions if not present as pass conditions
FUNCTION(add_default_fail_conditions name)
    GET_PROPERTY(
        EXPRESSIONS_FAIL
        TEST ${name}
        PROPERTY FAIL_REGULAR_EXPRESSION)
    IF(NOT EXPRESSIONS_FAIL)
        # Unless they are part of the pass condition, no WARNING, ERROR or FATAL logs should appear:
        GET_PROPERTY(
            EXPRESSIONS_PASS
            TEST ${name}
            PROPERTY PASS_REGULAR_EXPRESSION)
        IF(NOT "${EXPRESSIONS_PASS}" MATCHES "WARNING")
            SET_PROPERTY(
                TEST ${name}
                APPEND
                PROPERTY FAIL_REGULAR_EXPRESSION "WARNING")
        ENDIF()
        IF(NOT "${EXPRESSIONS_PASS}" MATCHES "ERROR")
            SET_PROPERTY(
                TEST ${name}
                APPEND
                PROPERTY FAIL_REGULAR_EXPRESSION "ERROR")
        ENDIF()
        IF(NOT "${EXPRESSIONS_PASS}" MATCHES "FATAL")
            SET_PROPERTY(
                TEST ${name}
                APPEND
                PROPERTY FAIL_REGULAR_EXPRESSION "FATAL")
        ENDIF()
    ENDIF()
ENDFUNCTION()

# Add a test to the unit test suite and parse its configuration file for options
FUNCTION(add_allpix_test)
    SET(options IGNORE_PASS_CONDITION)
    SET(oneValueArgs NAME FILE EXECUTABLE WORKING_DIRECTORY)
    SET(multiValueArgs DEPENDS)
    CMAKE_PARSE_ARGUMENTS(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Set default working directory
    IF(NOT DEFINED TEST_WORKING_DIRECTORY)
        # cmake-lint: disable=C0103
        SET(TEST_WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/testing")
    ENDIF()
    FILE(MAKE_DIRECTORY ${TEST_WORKING_DIRECTORY})

    # Default executable is "allpix"
    IF(NOT DEFINED TEST_EXECUTABLE)
        # cmake-lint: disable=C0103
        SET(TEST_EXECUTABLE "allpix")
    ENDIF()

    # Allow the test to specify additional module CLI parameters:
    FILE(STRINGS ${TEST_FILE} OPTS REGEX "#OPTION ")
    FOREACH(opt ${OPTS})
        STRING(REPLACE "#OPTION " "" opt "${opt}")
        SET(clioptions "${clioptions} -o ${opt}")
    ENDFOREACH()
    # Allow the test to specify additional geometry CLI parameters:
    FILE(STRINGS ${TEST_FILE} OPTS REGEX "#DETOPTION ")
    FOREACH(opt ${OPTS})
        STRING(REPLACE "#DETOPTION " "" opt "${opt}")
        SET(clioptions "${clioptions} -g ${opt}")
    ENDFOREACH()
    # Allow the test to specify additional CLI parameters:
    FILE(STRINGS ${TEST_FILE} OPTS REGEX "#CLIOPTION ")
    FOREACH(opt ${OPTS})
        STRING(REPLACE "#CLIOPTION " "" opt "${opt}")
        SET(clioptions "${clioptions} ${opt}")
    ENDFOREACH()

    # Register the test for inclusion in the documentation:
    FILE(STRINGS ${TEST_FILE} DESC REGEX "#DESC ")
    LIST(LENGTH DESC listcount_desc)
    IF(listcount_desc EQUAL 0)
        MESSAGE(WARNING "Test ${TEST_NAME} does not provide a description")
    ELSEIF(listcount_desc GREATER 1)
        MESSAGE(FATAL_ERROR "More than one DESC expressions defined in test ${TEST_NAME}")
    ELSE()
        STRING(REPLACE "#DESC " "\n- `${TEST_NAME}`:\n  " DESC "${DESC}")
        LIST(APPEND TEST_DESCRIPTIONS ${DESC})
    ENDIF()
    SET(TEST_DESCRIPTIONS
        ${TEST_DESCRIPTIONS}
        PARENT_SCOPE)

    # Parse possible commands to be run before
    FILE(STRINGS ${TEST_FILE} OPTS REGEX "#BEFORE_SCRIPT ")
    FOREACH(opt ${OPTS})
        STRING(REPLACE "#BEFORE_SCRIPT " "" opt "${opt}")
        LIST(APPEND before_script ${opt})
    ENDFOREACH()

    ADD_TEST(
        NAME "${TEST_NAME}"
        WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY}
        COMMAND ${PROJECT_SOURCE_DIR}/etc/unittests/run_directory.sh "${TEST_NAME}"
                "${CMAKE_INSTALL_PREFIX}/bin/${TEST_EXECUTABLE} -c ${TEST_FILE} ${clioptions}" ${before_script})

    # Parse configuration file for pass/fail conditions:
    FILE(STRINGS ${TEST_FILE} PASS_LST_ REGEX "#PASS ")
    FILE(STRINGS ${TEST_FILE} FAIL_LST_ REGEX "#FAIL ")

    # Check for number of pass or fail conditions - we should have at least one of them
    LIST(LENGTH PASS_LST_ listcount_pass)
    LIST(LENGTH FAIL_LST_ listcount_fail)
    IF(listcount_pass EQUAL 0 AND listcount_fail EQUAL 0)
        MESSAGE(FATAL_ERROR "Neither PASS nor FAIL defined for test \"${TEST_NAME}\"")
    ENDIF()

    IF(NOT TEST_IGNORE_PASS_CONDITION)
        # Escape possible regex patterns in the expected output:
        FOREACH(pass ${PASS_LST_})
            ESCAPE_REGEX("${pass}" pass)
            SET_PROPERTY(
                TEST ${TEST_NAME}
                APPEND
                PROPERTY PASS_REGULAR_EXPRESSION "${pass}")
        ENDFOREACH()
    ENDIF()
    FOREACH(fail ${FAIL_LST_})
        ESCAPE_REGEX("${fail}" fail)
        SET_PROPERTY(
            TEST ${TEST_NAME}
            APPEND
            PROPERTY FAIL_REGULAR_EXPRESSION "${fail}")
    ENDFOREACH()

    # Add default fail conditions
    ADD_DEFAULT_FAIL_CONDITIONS(${TEST_NAME})

    # Some tests might depend on others:
    FILE(STRINGS ${TEST_FILE} DEPENDENCY REGEX "#DEPENDS ")
    IF(DEPENDENCY)
        STRING(REPLACE "#DEPENDS " "" DEPENDENCY "${DEPENDENCY}")
        SET_PROPERTY(TEST ${TEST_NAME} PROPERTY DEPENDS "${DEPENDENCY}")
    ENDIF()
    FOREACH(depend ${TEST_DEPENDS})
        SET_PROPERTY(TEST ${TEST_NAME} PROPERTY DEPENDS "${depend}")
    ENDFOREACH()

    # Add individual timeout criteria:
    FILE(STRINGS ${TEST_FILE} TESTTIMEOUT REGEX "#TIMEOUT ")
    IF(TESTTIMEOUT)
        STRING(REPLACE "#TIMEOUT " "" TESTTIMEOUT "${TESTTIMEOUT}")
        SET_PROPERTY(TEST ${TEST_NAME} PROPERTY TIMEOUT_AFTER_MATCH "${TESTTIMEOUT}" "Starting event loop")
    ENDIF()

    # Allow to add test labels
    FILE(STRINGS ${TEST_FILE} TESTLABEL REGEX "#LABEL ")
    IF(TESTLABEL)
        STRING(REPLACE "#LABEL " "" TESTLABEL "${TESTLABEL}")
        SET_PROPERTY(TEST ${TEST_NAME} PROPERTY LABELS "${TESTLABEL}")
    ENDIF()

    # Add required files if specified
    FILE(STRINGS ${TEST_FILE} TESTDATA REGEX "#DATA ")
    IF(TESTDATA)
        STRING(REPLACE "#DATA " "" TESTDATA "${TESTDATA}")
        STRING(REPLACE " " ";" TESTDATA ${TESTDATA})
        SET_PROPERTY(TEST ${TEST_NAME} PROPERTY REQUIRED_FILES "${TESTDATA}")
    ENDIF()
ENDFUNCTION()

# Macro for adding module tests to CTest
MACRO(ALLPIX_MODULE_TESTS name directory)
    # Get the name of the module
    GET_FILENAME_COMPONENT(_allpix_module_dir ${CMAKE_CURRENT_SOURCE_DIR} NAME)

    SET(TEST_BASE_DIR "${PROJECT_BINARY_DIR}/testing")
    IF(TEST_MODULES)
        FILE(
            GLOB AUX_FILES_MODULES
            RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            ${directory}/[!00-99]*)

        # Copy and configure auxiliary files
        FOREACH(file ${AUX_FILES_MODULES})
            CONFIGURE_FILE(${file} "${CMAKE_CURRENT_BINARY_DIR}/${file}" @ONLY)
        ENDFOREACH()

        FILE(
            GLOB TEST_LIST_MODULES
            RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            ${directory}/[00-99]*.conf)

        # Copy and configure tests
        FOREACH(test ${TEST_LIST_MODULES})
            GET_FILENAME_COMPONENT(title ${test} NAME_WE)
            SET(TEST_DIR "${TEST_BASE_DIR}/modules/${_allpix_module_dir}/${title}")

            CONFIGURE_FILE(${test} "${CMAKE_CURRENT_BINARY_DIR}/${test}" @ONLY)
            ADD_ALLPIX_TEST(NAME "modules/${_allpix_module_dir}/${title}" FILE ${CMAKE_CURRENT_BINARY_DIR}/${test}
                            WORKING_DIRECTORY ${TEST_BASE_DIR})

            GET_PROPERTY(old_count GLOBAL PROPERTY COUNT_TESTS_MODULES)
            MATH(EXPR new_count "${old_count}+1")
            SET_PROPERTY(GLOBAL PROPERTY COUNT_TESTS_MODULES "${new_count}")
        ENDFOREACH()

        # Register that this module has tests:
        IF(TEST_LIST_MODULES)
            SET(_MODULES_WITH_TESTS
                "${_allpix_module_dir};${_MODULES_WITH_TESTS}"
                CACHE INTERNAL "MODULES_WITH_TESTS")
        ENDIF()

        # Append list of test descriptions to global property
        GET_PROPERTY(tmp GLOBAL PROPERTY MODULES_TEST_DESCRIPTIONS)
        FOREACH(item ${TEST_DESCRIPTIONS})
            SET(tmp "${tmp} ${item}")
        ENDFOREACH()
        SET_PROPERTY(GLOBAL PROPERTY MODULES_TEST_DESCRIPTIONS "${tmp}")
    ENDIF()
ENDMACRO()

# Provide default install target for the module
MACRO(ALLPIX_MODULE_INSTALL name)
    INSTALL(
        TARGETS ${name}
        COMPONENT modules
        EXPORT Allpix
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
ENDMACRO()

# Enable Geant4 interface library requirement
MACRO(ALLPIX_MODULE_REQUIRE_GEANT4_INTERFACE name)
    SET(options REQUIRED)
    SET(oneValueArgs "")
    SET(multiValueArgs COMPONENTS)
    CMAKE_PARSE_ARGUMENTS(GEANT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    SET(ALLPIX_BUILD_GEANT4_INTERFACE
        "ON"
        CACHE BOOL "Build Geant4 interface library" FORCE)
    TARGET_LINK_LIBRARIES(${name} ${ALLPIX_GEANT4_INTERFACE})

    IF(GEANT_REQUIRED)
        FIND_PACKAGE(Geant4 REQUIRED COMPONENTS "${GEANT_COMPONENTS}")
    ELSE()
        FIND_PACKAGE(Geant4 COMPONENTS "${GEANT_COMPONENTS}")
    ENDIF()
    IF(NOT Geant4_FOUND)
        MESSAGE(FATAL_ERROR "Could not find Geant4, make sure to source the Geant4 environment\n"
                            "$ source YOUR_GEANT4_DIR/bin/geant4.sh")
    ENDIF()

    FOREACH(component ${GEANT_COMPONENTS})
        IF(NOT ${Geant4_${component}_FOUND})
            MESSAGE(FATAL_ERROR "Could not find Geant4 component ${component}, make sure the installed Geant4 version"
                                "has this component available or switch off building this module")
        ENDIF()
    ENDFOREACH()

    # Add "geant4.sh" as runtime dependency for setup.sh file:
    ADD_RUNTIME_DEP(geant4.sh)

    # Add Geant4 flags before our own flags
    ADD_DEFINITIONS(${Geant4_DEFINITIONS})
    SEPARATE_ARGUMENTS(Geant4_CXX_FLAGS NATIVE_COMMAND)
    TARGET_COMPILE_OPTIONS(${name} PRIVATE ${Geant4_CXX_FLAGS})
    IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
        SEPARATE_ARGUMENTS(Geant4_CXX_FLAGS_DEBUG NATIVE_COMMAND)
        TARGET_COMPILE_OPTIONS(${name} PRIVATE ${Geant4_CXX_FLAGS_DEBUG})
    ELSEIF(CMAKE_BUILD_TYPE MATCHES RELEASE)
        SEPARATE_ARGUMENTS(Geant4_CXX_FLAGS_RELEASE NATIVE_COMMAND)
        TARGET_COMPILE_OPTIONS(${name} PRIVATE ${Geant4_CXX_FLAGS_RELEASE})
    ENDIF()

    # Add GDML flag if supported
    IF(Geant4_gdml_FOUND)
        ADD_DEFINITIONS(-DGeant4_GDML)
    ENDIF()

    # Include Geant4 directories (NOTE Geant4_USE_FILE is not used!)
    TARGET_INCLUDE_DIRECTORIES(${MODULE_NAME} SYSTEM PRIVATE ${Geant4_INCLUDE_DIRS})

    # Add Geant4 libraries
    TARGET_LINK_LIBRARIES(${MODULE_NAME} ${Geant4_LIBRARIES})
ENDMACRO()

# Macro to set up ROOT:: targets so that we can use the same code for root 6.8 and for root 6.10 and beyond
# Inpsired by CMake build system of DD4Hep
MACRO(ALLPIX_SETUP_ROOT_TARGETS)

    #ROOT CXX Flags are a string with quotes, not a list, so we need to convert to a list...
    STRING(REPLACE " " ";" ALLXPIX_ROOT_CXX_FLAGS ${ROOT_CXX_FLAGS})

    IF(NOT TARGET ROOT::Core)
        #in ROOT before 6.10 there is no ROOT namespace, so we create ROOT::Core ourselves
        ADD_LIBRARY(ROOT::Core INTERFACE IMPORTED GLOBAL)
        SET_TARGET_PROPERTIES(ROOT::Core PROPERTIES INTERFACE_COMPILE_OPTIONS "${ALLXPIX_ROOT_CXX_FLAGS}"
                                                    INTERFACE_INCLUDE_DIRECTORIES ${ROOT_INCLUDE_DIRS})
        # there is also no dependency between the targets
        TARGET_LINK_LIBRARIES(ROOT::Core INTERFACE Core)
        # we list here the targets we use, as later versions of root have the namespace, we do not have to to this for ever
        FOREACH(
            lib
            Geom
            GenVector
            Graf3d
            RIO
            MathCore
            Tree
            Hist
            GuiBld)
            IF(TARGET ${lib})
                ADD_LIBRARY(ROOT::${lib} INTERFACE IMPORTED GLOBAL)
                TARGET_LINK_LIBRARIES(ROOT::${lib} INTERFACE ${lib} ROOT::Core)
            ENDIF()
        ENDFOREACH()
    ELSEIF(${ROOT_VERSION} VERSION_GREATER_EQUAL 6.12 AND ${ROOT_VERSION} VERSION_LESS 6.14)
        # Root 6.12 exports ROOT::Core, but does not assign include directories to the target
        SET_TARGET_PROPERTIES(ROOT::Core PROPERTIES INTERFACE_COMPILE_OPTIONS "${ALLXPIX_ROOT_CXX_FLAGS}"
                                                    INTERFACE_INCLUDE_DIRECTORIES ${ROOT_INCLUDE_DIRS})
    ENDIF()

ENDMACRO()
