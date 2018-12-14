# Retrieve the allpix version string from git describe
FUNCTION(get_version PROJECT_VERSION)
    # Check if this is a source tarball build
    IF(NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)
        SET(SOURCE_PACKAGE 1)
    ENDIF(NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)

    # Set package version
    IF(NOT SOURCE_PACKAGE)
        SET(TAG_FOUND FALSE)

        # Get the version from last git tag plus number of additional commits:
        FIND_PACKAGE(Git QUIET)
        IF(GIT_FOUND)
            EXEC_PROGRAM(git ${CMAKE_CURRENT_SOURCE_DIR} ARGS describe --tags HEAD OUTPUT_VARIABLE ${PROJECT_VERSION})
            STRING(REGEX REPLACE "^release-" "" ${PROJECT_VERSION} ${${PROJECT_VERSION}})
            STRING(REGEX REPLACE "([v0-9.]+)-([0-9]+)-([A-Za-z0-9]+)" "\\1+\\2^\\3" ${PROJECT_VERSION} ${${PROJECT_VERSION}})
            EXEC_PROGRAM(git ARGS status --porcelain ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE PROJECT_STATUS)
            IF(PROJECT_STATUS STREQUAL "")
                MESSAGE(STATUS "Git project directory is clean.")
            ELSE(PROJECT_STATUS STREQUAL "")
                MESSAGE(STATUS "Git project directory is dirty:\n ${PROJECT_STATUS}.")
            ENDIF(PROJECT_STATUS STREQUAL "")

            # Check if commit flag has been set by the CI:
            IF(DEFINED ENV{CI_COMMIT_TAG})
                MESSAGE(STATUS "Found CI tag flag, building tagged version")
                SET(TAG_FOUND TRUE)
            ENDIF()
        ELSE(GIT_FOUND)
            MESSAGE(STATUS "Git repository present, but could not find git executable.")
        ENDIF(GIT_FOUND)
    ELSE(NOT SOURCE_PACKAGE)
        # If we don't have git we can not really do anything
        MESSAGE(STATUS "Source tarball build - no repository present.")
        SET(TAG_FOUND TRUE)
    ENDIF(NOT SOURCE_PACKAGE)

    # Set the project version in the parent scope
    SET(TAG_FOUND ${TAG_FOUND} PARENT_SCOPE)

    # Set the project version in the parent scope
    SET(${PROJECT_VERSION} ${${PROJECT_VERSION}} PARENT_SCOPE)
ENDFUNCTION()

FUNCTION(add_runtime_dep RUNTIME_NAME)
    GET_FILENAME_COMPONENT(THISPROG ${RUNTIME_NAME} PROGRAM)
    LIST(APPEND ALLPIX_RUNTIME_DEPS ${THISPROG})
    LIST(REMOVE_DUPLICATES ALLPIX_RUNTIME_DEPS)
    SET(ALLPIX_RUNTIME_DEPS ${ALLPIX_RUNTIME_DEPS} CACHE INTERNAL "ALLPIX_RUNTIME_DEPS")
ENDFUNCTION()

FUNCTION(add_runtime_lib RUNTIME_NAME)
    FOREACH(name ${RUNTIME_NAME})
        GET_FILENAME_COMPONENT(THISLIB ${name} DIRECTORY)
        LIST(APPEND ALLPIX_RUNTIME_LIBS ${THISLIB})
        LIST(REMOVE_DUPLICATES ALLPIX_RUNTIME_LIBS)
        SET(ALLPIX_RUNTIME_LIBS ${ALLPIX_RUNTIME_LIBS} CACHE INTERNAL "ALLPIX_RUNTIME_LIBS")
    ENDFOREACH()
ENDFUNCTION()

FUNCTION(GET_TEST_REGEX INP OUTPUT_PASS OUTPUT_FAIL)
    IF(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        FILE(STRINGS ${INP} OUTPUT_PASS_ REGEX "#PASSOSX ")
        FILE(STRINGS ${INP} OUTPUT_FAIL_ REGEX "#FAILOSX ")
    ENDIF()

    IF(NOT OUTPUT_PASS_)
        FILE(STRINGS ${INP} OUTPUT_PASS_ REGEX "#PASS ")
    ENDIF()
    IF(NOT OUTPUT_FAIL_)
        FILE(STRINGS ${INP} OUTPUT_FAIL_ REGEX "#FAIL ")
    ENDIF()

    # Check for number of arguments - should only be one:
    LIST(LENGTH OUTPUT_PASS_ LISTCOUNT_PASS)
    LIST(LENGTH OUTPUT_FAIL_ LISTCOUNT_FAIL)
    IF(LISTCOUNT_PASS GREATER 1)
        MESSAGE(FATAL_ERROR "More than one PASS expressions defined in test ${INP}")
    ENDIF()
    IF(LISTCOUNT_FAIL GREATER 1)
        MESSAGE(FATAL_ERROR "More than one FAIL expressions defined in test ${INP}")
    ENDIF()

    # Escape possible regex patterns in the expected output:
    ESCAPE_REGEX("${OUTPUT_PASS_}" OUTPUT_PASS_)
    ESCAPE_REGEX("${OUTPUT_FAIL_}" OUTPUT_FAIL_)

    SET(${OUTPUT_PASS} "${OUTPUT_PASS_}" PARENT_SCOPE)
    SET(${OUTPUT_FAIL} "${OUTPUT_FAIL_}" PARENT_SCOPE)
ENDFUNCTION()

FUNCTION(ESCAPE_REGEX INP OUTPUT)
    # Escape possible regex patterns in the expected output:
    STRING(REPLACE "#PASS " "" _TMP_STR "${INP}")
    STRING(REPLACE "#FAIL " "" _TMP_STR "${_TMP_STR}")
    STRING(REPLACE "#PASSOSX " "" _TMP_STR "${_TMP_STR}")
    STRING(REPLACE "#FAILOSX " "" _TMP_STR "${_TMP_STR}")
    STRING(REPLACE "\\" "\\\\" _TMP_STR "${_TMP_STR}")
    STRING(REPLACE "?" "\\?" _TMP_STR "${_TMP_STR}")
    STRING(REPLACE "+" "\\+" _TMP_STR "${_TMP_STR}")
    STRING(REPLACE "*" "\\*" _TMP_STR "${_TMP_STR}")
    STRING(REPLACE "(" "\\\(" _TMP_STR "${_TMP_STR}")
    STRING(REPLACE ")" "\\\)" _TMP_STR "${_TMP_STR}")
    STRING(REPLACE "[" "\\\[" _TMP_STR "${_TMP_STR}")
    STRING(REPLACE "]" "\\\]" _TMP_STR "${_TMP_STR}")
    SET(${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
ENDFUNCTION()

FUNCTION(ADD_ALLPIX_TEST TEST)
    # Allow the test to specify additional module CLI parameters:
    FILE(STRINGS ${TEST} OPTS REGEX "#OPTION ")
    FOREACH(OPT ${OPTS})
        STRING(REPLACE "#OPTION " "" OPT "${OPT}")
        SET(CLIOPTIONS "${CLIOPTIONS} -o ${OPT}")
    ENDFOREACH()
    # Allow the test to specify additional geometry CLI parameters:
    FILE(STRINGS ${TEST} OPTS REGEX "#DETOPTION ")
    FOREACH(OPT ${OPTS})
        STRING(REPLACE "#DETOPTION " "" OPT "${OPT}")
        SET(CLIOPTIONS "${CLIOPTIONS} -g ${OPT}")
    ENDFOREACH()

    ADD_TEST(NAME ${TEST}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_directory.sh "output/${TEST}" "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_CURRENT_SOURCE_DIR}/${TEST} ${CLIOPTIONS}"
    )

    # Parse configuration file for pass/fail conditions:
    GET_TEST_REGEX(${TEST} EXPRESSIONS_PASS EXPRESSIONS_FAIL)
    IF(EXPRESSIONS_PASS)
        SET_TESTS_PROPERTIES(${TEST} PROPERTIES PASS_REGULAR_EXPRESSION "${EXPRESSIONS_PASS}")
    ENDIF()
    IF(EXPRESSIONS_FAIL)
        SET_TESTS_PROPERTIES(${TEST} PROPERTIES FAIL_REGULAR_EXPRESSION "${EXPRESSIONS_FAIL}")
    ENDIF()

    # Some tests might depend on others:
    FILE(STRINGS ${TEST} DEPENDENCY REGEX "#DEPENDS ")
    IF(DEPENDENCY)
        STRING(REPLACE "#DEPENDS " "" DEPENDENCY "${DEPENDENCY}")
        SET_TESTS_PROPERTIES(${TEST} PROPERTIES DEPENDS "${DEPENDENCY}")
    ENDIF()

    # Add individual timeout criteria:
    FILE(STRINGS ${TEST} TESTTIMEOUT REGEX "#TIMEOUT ")
    IF(TESTTIMEOUT)
        STRING(REPLACE "#TIMEOUT " "" TESTTIMEOUT "${TESTTIMEOUT}")
        SET_TESTS_PROPERTIES(${TEST} PROPERTIES TIMEOUT "${TESTTIMEOUT}")
    ENDIF()

ENDFUNCTION()
