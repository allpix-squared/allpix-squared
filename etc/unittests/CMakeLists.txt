##############################
# Module functionality tests #
##############################

OPTION(TEST_MODULES "Perform unit tests to ensure module functionality?" ON)

IF(TEST_MODULES)
    FILE(
        GLOB TEST_LIST_MODULES
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        test_modules/test_*)
    LIST(LENGTH TEST_LIST_MODULES NUM_TEST_MODULES)
    MESSAGE(STATUS "Unit tests: ${NUM_TEST_MODULES} module functionality tests")
    FOREACH(test ${TEST_LIST_MODULES})
        ADD_ALLPIX_TEST(${test})
    ENDFOREACH()
ELSE()
    MESSAGE(STATUS "Unit tests: module functionality tests deactivated.")
ENDIF()

###############################
# Framework performance tests #
###############################

OPTION(TEST_PERFORMANCE "Perform unit tests to ensure framework performance?" ON)

IF(TEST_PERFORMANCE)
    FILE(
        GLOB TEST_LIST_PERFORMANCE
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        test_performance/test_*)
    LIST(LENGTH TEST_LIST_PERFORMANCE NUM_TEST_PERFORMANCE)
    MESSAGE(STATUS "Unit tests: ${NUM_TEST_PERFORMANCE} performance tests")
    FOREACH(test ${TEST_LIST_PERFORMANCE})
        ADD_ALLPIX_TEST(${test})
    ENDFOREACH()
ELSE()
    MESSAGE(STATUS "Unit tests: performance tests deactivated.")
ENDIF()

######################################
# Core framework functionality tests #
######################################

OPTION(TEST_CORE "Perform unit tests to ensure framework core functionality?" ON)

IF(TEST_CORE)
    FILE(
        GLOB TEST_LIST_CORE
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        test_core/test_*)
    LIST(LENGTH TEST_LIST_CORE NUM_TEST_CORE)
    MESSAGE(STATUS "Unit tests: ${NUM_TEST_CORE} framework core functionality tests")
    FOREACH(test ${TEST_LIST_CORE})
        ADD_ALLPIX_TEST(${test})
    ENDFOREACH()
ELSE()
    MESSAGE(STATUS "Unit tests: framework core functionality tests deactivated.")
ENDIF()
