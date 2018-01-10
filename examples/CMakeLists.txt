###########################################
# Reference tests for configuration files #
# provided as example and in manual       #
###########################################

ADD_TEST(NAME example
COMMAND ${CMAKE_SOURCE_DIR}/etc/unittests/run_directory.sh "test_example" "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_SOURCE_DIR}/examples/example.conf -o number_of_events=10")
ADD_TEST(NAME manual
COMMAND ${CMAKE_SOURCE_DIR}/etc/unittests/run_directory.sh "test_manual" "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_SOURCE_DIR}/etc/manual.conf")

OPTION(TEST_EXAMPLES "Perform unit tests to ensure example validity?" ON)

IF(TEST_EXAMPLES)
    FILE(GLOB TEST_LIST_EXAMPLES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
    FOREACH(TEST ${TEST_LIST_EXAMPLES})
        IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST}/${TEST}.conf")
            ADD_TEST(NAME "examples/${TEST}"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMAND ${CMAKE_SOURCE_DIR}/etc/unittests/run_directory.sh "output/${TEST}" "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_SOURCE_DIR}/examples/${TEST}/${TEST}.conf -o number_of_events=10"
            )
        ENDIF()
    ENDFOREACH()
ELSE()
    MESSAGE(STATUS "Unit tests: example validity tests deactivated.")
ENDIF()
