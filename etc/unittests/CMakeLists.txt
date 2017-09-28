
###########################################
# Reference tests for configuration files #
# provided as example and in manual       #
###########################################

ADD_TEST(NAME example
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_directory.sh "test_example" "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_SOURCE_DIR}/etc/example.conf")
ADD_TEST(NAME manual
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_directory.sh "test_manual" "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_SOURCE_DIR}/etc/manual.conf")

##############################
# Module functionality tests #
##############################

OPTION(TEST_MODULES "Perform unit tests to ensure module functionality?" ON)

IF(TEST_MODULES)
    FILE(GLOB TEST_LIST_MODULES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test_modules/test_*)
    LIST(LENGTH TEST_LIST_MODULES NUM_TEST_MODULES)
    MESSAGE(STATUS "Unit tests: ${NUM_TEST_MODULES} module functionality tests")
    FOREACH(TEST ${TEST_LIST_MODULES})
        ADD_TEST(NAME ${TEST}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_directory.sh "output/${TEST}" "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_CURRENT_SOURCE_DIR}/${TEST}"
        )
        FILE(STRINGS ${TEST} EXPRESSIONS REGEX "#REGEX ")

        # Escape possible regex patterns in the expected output:
        STRING(REPLACE "#REGEX " "" EXPRESSIONS "${EXPRESSIONS}")
        STRING(REPLACE "\\" "\\\\" EXPRESSIONS "${EXPRESSIONS}")
        STRING(REPLACE "?" "\\?" EXPRESSIONS "${EXPRESSIONS}")
        STRING(REPLACE "+" "\\+" EXPRESSIONS "${EXPRESSIONS}")
        STRING(REPLACE "*" "\\*" EXPRESSIONS "${EXPRESSIONS}")
        STRING(REPLACE "(" "\\\(" EXPRESSIONS "${EXPRESSIONS}")
        STRING(REPLACE ")" "\\\)" EXPRESSIONS "${EXPRESSIONS}")
        STRING(REPLACE "[" "\\\[" EXPRESSIONS "${EXPRESSIONS}")
        STRING(REPLACE "]" "\\\]" EXPRESSIONS "${EXPRESSIONS}")

        SET_TESTS_PROPERTIES(${TEST} PROPERTIES PASS_REGULAR_EXPRESSION "${EXPRESSIONS}")

        # Some tests might depend on others:
        FILE(STRINGS ${TEST} DEPENDENCY REGEX "#DEPENDS ")
        IF(DEPENDENCY)
            STRING(REPLACE "#DEPENDS " "" DEPENDENCY "${DEPENDENCY}")
            SET_TESTS_PROPERTIES(${TEST} PROPERTIES DEPENDS "${DEPENDENCY}")
        ENDIF()

    ENDFOREACH()
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
        ADD_TEST(NAME ${TEST}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_directory.sh "output/${TEST}" "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_CURRENT_SOURCE_DIR}/${TEST}"
        )

        # Add individual timeout criteria:
        FILE(STRINGS ${TEST} EXPRESSIONS REGEX "#TIMEOUT ")
        STRING(REPLACE "#TIMEOUT " "" EXPRESSIONS "${EXPRESSIONS}")
        SET_TESTS_PROPERTIES(${TEST} PROPERTIES TIMEOUT "${EXPRESSIONS}")

        # Some tests might depend on others:
        FILE(STRINGS ${TEST} DEPENDENCY REGEX "#DEPENDS ")
        IF(DEPENDENCY)
            STRING(REPLACE "#DEPENDS " "" DEPENDENCY "${DEPENDENCY}")
            SET_TESTS_PROPERTIES(${TEST} PROPERTIES DEPENDS "${DEPENDENCY}")
        ENDIF()

    ENDFOREACH()
ENDIF()
