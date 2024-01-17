# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

###########################################
# Reference tests for configuration files #
# provided as example and in manual       #
###########################################

SET(DEFAULT_CLI_TEST_ARGS "-o number_of_events=10 -o random_seed=0")
ADD_TEST(NAME "examples/example"
         COMMAND ${CMAKE_SOURCE_DIR}/etc/unittests/run_directory.sh "test_example"
                 "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_SOURCE_DIR}/examples/example.conf ${DEFAULT_CLI_TEST_ARGS}")
ADD_DEFAULT_FAIL_CONDITIONS("examples/example")
ADD_TEST(NAME "examples/manual"
         COMMAND ${CMAKE_SOURCE_DIR}/etc/unittests/run_directory.sh "test_manual"
                 "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_SOURCE_DIR}/etc/manual.conf ${DEFAULT_CLI_TEST_ARGS}")
ADD_DEFAULT_FAIL_CONDITIONS("examples/manual")

IF(TEST_EXAMPLES)
    FILE(
        GLOB TEST_LIST_EXAMPLES
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/*)
    LIST(LENGTH TEST_LIST_EXAMPLES NUM_TEST_EXAMPLES)
    MESSAGE(STATUS "Unit tests: ${NUM_TEST_EXAMPLES} examples")
    FOREACH(test ${TEST_LIST_EXAMPLES})
        IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${test}/${test}.conf")
            # cmake-lint: disable=C0301
            SET(TEST_NAME "examples/${test}")

            # Add ad-hoc cli arguments for some examples:
            IF(${test} MATCHES "replay")
                SET(ADDITIONAL_CLI_ARGS
                    "${ADDITIONAL_CLI_ARGS} -o ROOTObjectReader.file_name=${CMAKE_BINARY_DIR}/output/fast_simulation/output/output_fast_simulation.root"
                )
                SET(DEPENDENCY "examples/fast_simulation")
            ENDIF()

            ADD_TEST(
                NAME "${TEST_NAME}"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMAND
                    ${CMAKE_SOURCE_DIR}/etc/unittests/run_directory.sh "output/${test}"
                    "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_SOURCE_DIR}/examples/${test}/${test}.conf ${DEFAULT_CLI_TEST_ARGS} ${ADDITIONAL_CLI_ARGS}"
            )
            SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES DEPENDS "${DEPENDENCY}")

            IF(${test} MATCHES "radial_strip" OR ${test} MATCHES "atlas_itk_petal")
                SET_TESTS_PROPERTIES(${TEST_NAME} PROPERTIES FAIL_REGULAR_EXPRESSION "FATAL")
            ENDIF()

            ADD_DEFAULT_FAIL_CONDITIONS(${TEST_NAME})
        ENDIF()
    ENDFOREACH()
ELSE()
    MESSAGE(STATUS "Unit tests: example validity tests deactivated.")
ENDIF()
