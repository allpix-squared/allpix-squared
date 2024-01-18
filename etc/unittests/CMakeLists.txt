# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

###############################
# Framework performance tests #
###############################

IF(TEST_PERFORMANCE)
    FILE(
        GLOB TEST_LIST_PERFORMANCE
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        test_performance/test_*)
    LIST(LENGTH TEST_LIST_PERFORMANCE NUM_TEST_PERFORMANCE)
    SET_PROPERTY(GLOBAL PROPERTY COUNT_TESTS_PERFORMANCE "${NUM_TEST_PERFORMANCE}")
    FOREACH(test ${TEST_LIST_PERFORMANCE})
        GET_FILENAME_COMPONENT(ext ${test} LAST_EXT)
        IF("${ext}" STREQUAL ".conf")
            GET_FILENAME_COMPONENT(title ${test} NAME_WE)
            ADD_ALLPIX_TEST(NAME "performance/${title}" FILE ${CMAKE_CURRENT_SOURCE_DIR}/${test})
        ENDIF()
    ENDFOREACH()
    SET_PROPERTY(GLOBAL PROPERTY PERF_TEST_DESCRIPTIONS "${TEST_DESCRIPTIONS}")
    SET(TEST_DESCRIPTIONS "")
ENDIF()

######################################
# Core framework functionality tests #
######################################

IF(TEST_CORE)
    FILE(
        GLOB TEST_LIST_CORE
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        test_core/test_*)
    LIST(LENGTH TEST_LIST_CORE NUM_TEST_CORE)
    SET_PROPERTY(GLOBAL PROPERTY COUNT_TESTS_CORE "${NUM_TEST_CORE}")
    FOREACH(test ${TEST_LIST_CORE})
        GET_FILENAME_COMPONENT(ext ${test} LAST_EXT)
        IF("${ext}" STREQUAL ".conf")
            GET_FILENAME_COMPONENT(title ${test} NAME_WE)
            ADD_ALLPIX_TEST(NAME "core/${title}" FILE ${CMAKE_CURRENT_SOURCE_DIR}/${test})
        ENDIF()
    ENDFOREACH()
    SET_PROPERTY(GLOBAL PROPERTY CORE_TEST_DESCRIPTIONS "${TEST_DESCRIPTIONS}")
    SET(TEST_DESCRIPTIONS "")
ENDIF()
