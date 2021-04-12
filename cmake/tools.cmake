# Retrieve the allpix version string from git describe
FUNCTION(get_version project_version)
    # Check if this is a source tarball build
    IF(NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)
        SET(source_package 1)
    ENDIF(NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)

    # Set package version
    IF(NOT source_package)
        SET(tag_found FALSE)

        # Get the version from last git tag plus number of additional commits:
        FIND_PACKAGE(Git QUIET)
        IF(GIT_FOUND)
            EXEC_PROGRAM(
                git ${CMAKE_CURRENT_SOURCE_DIR}
                ARGS describe --tags HEAD
                OUTPUT_VARIABLE ${project_version})
            STRING(REGEX REPLACE "^release-" "" ${project_version} ${${project_version}})
            STRING(REGEX REPLACE "([v0-9.]+)-([0-9]+)-([A-Za-z0-9]+)" "\\1+\\2^\\3" ${project_version} ${${project_version}})
            EXEC_PROGRAM(
                git ARGS
                status --porcelain ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE PROJECT_STATUS)
            IF(PROJECT_STATUS STREQUAL "")
                MESSAGE(STATUS "Git project directory is clean.")
            ELSE(PROJECT_STATUS STREQUAL "")
                MESSAGE(STATUS "Git project directory is dirty:\n ${PROJECT_STATUS}.")
            ENDIF(PROJECT_STATUS STREQUAL "")

            # Check if commit flag has been set by the CI:
            IF(DEFINED ENV{CI_COMMIT_TAG})
                MESSAGE(STATUS "Found CI tag flag, building tagged version")
                SET(tag_found TRUE)
            ENDIF()
        ELSE(GIT_FOUND)
            MESSAGE(STATUS "Git repository present, but could not find git executable.")
        ENDIF(GIT_FOUND)
    ELSE(NOT source_package)
        # If we don't have git we can not really do anything
        MESSAGE(STATUS "Source tarball build - no repository present.")
        SET(tag_found TRUE)
    ENDIF(NOT source_package)

    # Set the project version in the parent scope
    SET(TAG_FOUND
        ${tag_found}
        PARENT_SCOPE)

    # Set the project version in the parent scope
    SET(${project_version}
        ${${project_version}}
        PARENT_SCOPE)
ENDFUNCTION()

# Add runtime dependency requirement to the list for generating a bash source file
FUNCTION(add_runtime_dep runtime_name)
    GET_FILENAME_COMPONENT(THISPROG ${runtime_name} PROGRAM)
    LIST(APPEND _ALLPIX_RUNTIME_DEPS ${THISPROG})
    LIST(REMOVE_DUPLICATES _ALLPIX_RUNTIME_DEPS)
    SET(_ALLPIX_RUNTIME_DEPS
        ${_ALLPIX_RUNTIME_DEPS}
        CACHE INTERNAL "ALLPIX_RUNTIME_DEPS")
ENDFUNCTION()

# Add runtime-library requirement to the list for generating a bash source file
FUNCTION(add_runtime_lib runtime_name)
    FOREACH(name ${runtime_name})
        GET_FILENAME_COMPONENT(THISLIB ${name} DIRECTORY)
        LIST(APPEND _ALLPIX_RUNTIME_LIBS ${THISLIB})
        LIST(REMOVE_DUPLICATES _ALLPIX_RUNTIME_LIBS)
        SET(_ALLPIX_RUNTIME_LIBS
            ${_ALLPIX_RUNTIME_LIBS}
            CACHE INTERNAL "ALLPIX_RUNTIME_LIBS")
    ENDFOREACH()
ENDFUNCTION()

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
FUNCTION(add_allpix_test test)
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
        NAME ${test}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_directory.sh "output/${test}"
                "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_CURRENT_SOURCE_DIR}/${test} ${clioptions}")

    # Parse configuration file for pass/fail conditions:
    GET_TEST_REGEX(${test} EXPRESSIONS_PASS EXPRESSIONS_FAIL)
    IF(EXPRESSIONS_PASS)
        SET_PROPERTY(TEST ${test} PROPERTY PASS_REGULAR_EXPRESSION "${EXPRESSIONS_PASS}")
    ENDIF()
    IF(EXPRESSIONS_FAIL)
        SET_PROPERTY(TEST ${test} PROPERTY FAIL_REGULAR_EXPRESSION "${EXPRESSIONS_FAIL}")
    ENDIF()

    # Some tests might depend on others:
    FILE(STRINGS ${test} DEPENDENCY REGEX "#DEPENDS ")
    IF(DEPENDENCY)
        STRING(REPLACE "#DEPENDS " "" DEPENDENCY "${DEPENDENCY}")
        SET_PROPERTY(TEST ${test} PROPERTY DEPENDS "${DEPENDENCY}")
    ENDIF()

    # Add individual timeout criteria:
    FILE(STRINGS ${test} TESTTIMEOUT REGEX "#TIMEOUT ")
    IF(TESTTIMEOUT)
        STRING(REPLACE "#TIMEOUT " "" TESTTIMEOUT "${TESTTIMEOUT}")
        SET_PROPERTY(TEST ${test} PROPERTY TIMEOUT_AFTER_MATCH "${TESTTIMEOUT}" "Running event")
    ENDIF()

    # Allow to add test labels
    FILE(STRINGS ${test} TESTLABEL REGEX "#LABEL ")
    IF(TESTLABEL)
        STRING(REPLACE "#LABEL " "" TESTLABEL "${TESTLABEL}")
        SET_PROPERTY(TEST ${test} PROPERTY LABELS "${TESTLABEL}")
    ENDIF()

ENDFUNCTION()
