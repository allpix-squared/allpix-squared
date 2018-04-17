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
    # Allow the test to specify additional CLI parameters:
    FILE(STRINGS ${TEST} OPTS REGEX "#OPTION ")
    FOREACH(OPT ${OPTS})
        STRING(REPLACE "#OPTION " "" OPT "${OPT}")
        SET(CLIOPTIONS "${CLIOPTIONS} -o ${OPT}")
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

##############################
# Module functionality tests #
##############################

OPTION(TEST_MODULES "Perform unit tests to ensure module functionality?" ON)

IF(TEST_MODULES)
    FILE(GLOB TEST_LIST_MODULES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test_modules/test_*)
    LIST(LENGTH TEST_LIST_MODULES NUM_TEST_MODULES)
    MESSAGE(STATUS "Unit tests: ${NUM_TEST_MODULES} module functionality tests")
    FOREACH(TEST ${TEST_LIST_MODULES})
        ADD_ALLPIX_TEST(${TEST})
    ENDFOREACH()
ELSE()
    MESSAGE(STATUS "Unit tests: module functionality tests deactivated.")
ENDIF()

###############################
# Framework performance tests #
###############################

OPTION(TEST_PERFORMANCE "Perform unit tests to ensure framework performance?" ON)

IF(TEST_PERFORMANCE)
    FILE(GLOB TEST_LIST_PERFORMANCE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test_performance/test_*)
    LIST(LENGTH TEST_LIST_PERFORMANCE NUM_TEST_PERFORMANCE)
    MESSAGE(STATUS "Unit tests: ${NUM_TEST_PERFORMANCE} performance tests")
    FOREACH(TEST ${TEST_LIST_PERFORMANCE})
        ADD_ALLPIX_TEST(${TEST})
    ENDFOREACH()
ELSE()
    MESSAGE(STATUS "Unit tests: performance tests deactivated.")
ENDIF()

######################################
# Core framework functionality tests #
######################################

OPTION(TEST_CORE "Perform unit tests to ensure framework core functionality?" ON)

IF(TEST_CORE)
    FILE(GLOB TEST_LIST_CORE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test_core/test_*)
    LIST(LENGTH TEST_LIST_CORE NUM_TEST_CORE)
    MESSAGE(STATUS "Unit tests: ${NUM_TEST_CORE} framework core functionality tests")
    FOREACH(TEST ${TEST_LIST_CORE})
        ADD_ALLPIX_TEST(${TEST})
    ENDFOREACH()
ELSE()
    MESSAGE(STATUS "Unit tests: framework core functionality tests deactivated.")
ENDIF()
