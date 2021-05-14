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

    # Add the current directory as include directory
    TARGET_INCLUDE_DIRECTORIES(${${name}} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    # Set the special header flags and add the special dynamic implementation file
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE ALLPIX_MODULE_NAME=${_allpix_module_class})
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE ALLPIX_MODULE_HEADER="${_allpix_module_class}.hpp")

    # If modules are build externally, the path to the dynamic implementation changes and we need to link differently:
    IF(${ALLPIX_MODULE_EXTERNAL})
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

# Retrieve regular expressions for test output matching from the test's configuration file
FUNCTION(get_test_regex inp output_pass output_fail)
    IF(NOT OUTPUT_PASS_)
        FILE(STRINGS ${inp} OUTPUT_PASS_ REGEX "#PASS ")
    ENDIF()
    IF(NOT OUTPUT_FAIL_)
        FILE(STRINGS ${inp} OUTPUT_FAIL_ REGEX "#FAIL ")
    ENDIF()

    # Check for number of arguments - should only be one:
    LIST(LENGTH OUTPUT_PASS_ listcount_pass)
    LIST(LENGTH OUTPUT_FAIL_ listcount_fail)
    IF(listcount_pass GREATER 1)
        MESSAGE(FATAL_ERROR "More than one PASS expressions defined in test ${inp}")
    ENDIF()
    IF(listcount_fail GREATER 1)
        MESSAGE(FATAL_ERROR "More than one FAIL expressions defined in test ${inp}")
    ENDIF()

    # Escape possible regex patterns in the expected output:
    ESCAPE_REGEX("${OUTPUT_PASS_}" OUTPUT_PASS_)
    ESCAPE_REGEX("${OUTPUT_FAIL_}" OUTPUT_FAIL_)

    SET(${output_pass}
        "${OUTPUT_PASS_}"
        PARENT_SCOPE)
    SET(${output_fail}
        "${OUTPUT_FAIL_}"
        PARENT_SCOPE)
ENDFUNCTION()

# Escape regular expressions in the match strings of tests
FUNCTION(escape_regex inp output)
    # Escape possible regex patterns in the expected output:
    STRING(REPLACE "#PASS " "" _TMP_STR "${inp}")
    STRING(REPLACE "#FAIL " "" _TMP_STR "${_TMP_STR}")
    STRING(REGEX REPLACE "([][+.*()^])" "\\\\\\1" _TMP_STR "${_TMP_STR}")
    SET(${output}
        "${_TMP_STR}"
        PARENT_SCOPE)
ENDFUNCTION()

# Add a test to the unit test suite and parse its configuration file for options
FUNCTION(add_allpix_test test name)
    # Allow the test to specify additional module CLI parameters:
    FILE(STRINGS ${test} OPTS REGEX "#OPTION ")
    FOREACH(opt ${OPTS})
        STRING(REPLACE "#OPTION " "" opt "${opt}")
        SET(clioptions "${clioptions} -o ${opt}")
    ENDFOREACH()
    # Allow the test to specify additional geometry CLI parameters:
    FILE(STRINGS ${test} OPTS REGEX "#DETOPTION ")
    FOREACH(opt ${OPTS})
        STRING(REPLACE "#DETOPTION " "" opt "${opt}")
        SET(clioptions "${clioptions} -g ${opt}")
    ENDFOREACH()
    # Allow the test to specify additional CLI parameters:
    FILE(STRINGS ${test} OPTS REGEX "#CLIOPTION ")
    FOREACH(opt ${OPTS})
        STRING(REPLACE "#CLIOPTION " "" opt "${opt}")
        SET(clioptions "${clioptions} ${opt}")
    ENDFOREACH()

    ADD_TEST(
        NAME "${name}"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/etc/unittests
        COMMAND ${PROJECT_SOURCE_DIR}/etc/unittests/run_directory.sh "output/${name}"
                "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_CURRENT_SOURCE_DIR}/${test} ${clioptions}")

    # Parse configuration file for pass/fail conditions:
    GET_TEST_REGEX(${test} EXPRESSIONS_PASS EXPRESSIONS_FAIL)
    IF(EXPRESSIONS_PASS)
        SET_PROPERTY(TEST ${name} PROPERTY PASS_REGULAR_EXPRESSION "${EXPRESSIONS_PASS}")
    ENDIF()
    IF(EXPRESSIONS_FAIL)
        SET_PROPERTY(TEST ${name} PROPERTY FAIL_REGULAR_EXPRESSION "${EXPRESSIONS_FAIL}")
    ENDIF()

    # Some tests might depend on others:
    FILE(STRINGS ${test} DEPENDENCY REGEX "#DEPENDS ")
    IF(DEPENDENCY)
        STRING(REPLACE "#DEPENDS " "" DEPENDENCY "${DEPENDENCY}")
        SET_PROPERTY(TEST ${name} PROPERTY DEPENDS "${DEPENDENCY}")
    ENDIF()

    # Add individual timeout criteria:
    FILE(STRINGS ${test} TESTTIMEOUT REGEX "#TIMEOUT ")
    IF(TESTTIMEOUT)
        STRING(REPLACE "#TIMEOUT " "" TESTTIMEOUT "${TESTTIMEOUT}")
        SET_PROPERTY(TEST ${name} PROPERTY TIMEOUT_AFTER_MATCH "${TESTTIMEOUT}" "Running event")
    ENDIF()

    # Allow to add test labels
    FILE(STRINGS ${test} TESTLABEL REGEX "#LABEL ")
    IF(TESTLABEL)
        STRING(REPLACE "#LABEL " "" TESTLABEL "${TESTLABEL}")
        SET_PROPERTY(TEST ${name} PROPERTY LABELS "${TESTLABEL}")
    ENDIF()
ENDFUNCTION()

# Macro for adding module tests to CTest
MACRO(ALLPIX_MODULE_TESTS name directory)
    # Get the name of the module
    GET_FILENAME_COMPONENT(_allpix_module_dir ${CMAKE_CURRENT_SOURCE_DIR} NAME)

    IF(TEST_MODULES)
        FILE(
            GLOB TEST_LIST_MODULES
            RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            ${directory}/[00-99]*)

        FOREACH(test ${TEST_LIST_MODULES})
            GET_FILENAME_COMPONENT(title ${test} NAME_WE)
            ADD_ALLPIX_TEST(${test} "modules/${_allpix_module_dir}/${title}")

            GET_PROPERTY(old_count GLOBAL PROPERTY COUNT_TESTS_MODULES)
            MATH(EXPR new_count "${old_count}+1")
            SET_PROPERTY(GLOBAL PROPERTY COUNT_TESTS_MODULES "${new_count}")
        ENDFOREACH()

        # Register that this module has tests:
        IF(TEST_LIST_MODULES)
            SET(MODULES_WITH_TESTS "${_allpix_module_dir};${MODULES_WITH_TESTS}" CACHE INTERNAL "MODULES_WITH_TESTS")
        ENDIF()
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
    SET(ALLPIX_BUILD_GEANT4_INTERFACE
        "ON"
        CACHE BOOL "Build Geant4 interface library" FORCE)
    TARGET_LINK_LIBRARIES(${name} ${ALLPIX_GEANT4_INTERFACE})
ENDMACRO()

# Macro to set up Eigen3:: targets
MACRO(ALLPIX_SETUP_EIGEN_TARGETS)

    IF(NOT TARGET Eigen3::Eigen)
        ADD_LIBRARY(Eigen3::Eigen INTERFACE IMPORTED GLOBAL)
        SET_TARGET_PROPERTIES(Eigen3::Eigen PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${EIGEN3_INCLUDE_DIR})
    ENDIF()

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
