# Configuration file testing
add_test(NAME example
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_directory.sh "test_example" "$<TARGET_FILE:allpix> -c ${CMAKE_SOURCE_DIR}/etc/example.conf")
add_test(NAME manual
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_directory.sh "test_manual" "$<TARGET_FILE:allpix> -c ${CMAKE_SOURCE_DIR}/etc/manual.conf")
         
# Performance and comparison test
add_test(NAME check_performance
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_directory.sh "test_check_performance " "$<TARGET_FILE:allpix> -c ${CMAKE_SOURCE_DIR}/test/check.conf 
                                                                                         -l  ${CMAKE_BINARY_DIR}/output_check_performance.log")
