
###########################################
# Reference tests for configuration files #
# provided as example and in manual       #
###########################################

add_test(NAME example
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_directory.sh "test_example" "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_SOURCE_DIR}/etc/example.conf")
add_test(NAME manual
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_directory.sh "test_manual" "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_SOURCE_DIR}/etc/manual.conf")

# Performance and comparison test
add_test(NAME check_performance
COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_directory.sh "test_check_performance " "${CMAKE_INSTALL_PREFIX}/bin/allpix -c ${CMAKE_SOURCE_DIR}/etc/unittests/check.conf -l  ${CMAKE_BINARY_DIR}/output_check_performance.log")
