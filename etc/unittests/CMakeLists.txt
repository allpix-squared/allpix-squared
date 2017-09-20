
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

FILE(GLOB TEST_MODULES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test_modules/test_*)
LIST(LENGTH TEST_MODULES NUM_TEST_MODULES)
MESSAGE(STATUS "Unit tests: ${NUM_TEST_MODULES} module functionality tests")
FOREACH(TEST ${TEST_MODULES})
    ADD_TEST(NAME ${TEST}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_directory.sh "output/${TEST}" "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_CURRENT_SOURCE_DIR}/${TEST}"
    )
    FILE(STRINGS ${TEST} EXPRESSIONS REGEX "# ")

    # Escape possible regex patterns in the expected output:
    STRING(REPLACE "# " "" EXPRESSIONS "${EXPRESSIONS}")
    STRING(REPLACE "\\" "\\\\" EXPRESSIONS "${EXPRESSIONS}")
    STRING(REPLACE "?" "\\?" EXPRESSIONS "${EXPRESSIONS}")
    STRING(REPLACE "+" "\\+" EXPRESSIONS "${EXPRESSIONS}")
    STRING(REPLACE "*" "\\*" EXPRESSIONS "${EXPRESSIONS}")
    STRING(REPLACE "(" "\\\(" EXPRESSIONS "${EXPRESSIONS}")
    STRING(REPLACE ")" "\\\)" EXPRESSIONS "${EXPRESSIONS}")
    STRING(REPLACE "[" "\\\]" EXPRESSIONS "${EXPRESSIONS}")
    STRING(REPLACE "]" "\\\]" EXPRESSIONS "${EXPRESSIONS}")

    SET_TESTS_PROPERTIES(${TEST} PROPERTIES PASS_REGULAR_EXPRESSION "${EXPRESSIONS}")

    # Some tests might depend on others:
    FILE(STRINGS ${TEST} DEPENDENCY REGEX "#DEPENDS ")
    IF(DEPENDENCY)
        STRING(REPLACE "#DEPENDS " "" DEPENDENCY "${DEPENDENCY}")
        SET_TESTS_PROPERTIES(${TEST} PROPERTIES DEPENDS "${DEPENDENCY}")
    ENDIF()

ENDFOREACH()

###############################
# Framework performance tests #
###############################

ADD_TEST(NAME check_performance
COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_directory.sh "test_check_performance " "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_SOURCE_DIR}/etc/unittests/check.conf -l  ${CMAKE_BINARY_DIR}/output_check_performance.log")
