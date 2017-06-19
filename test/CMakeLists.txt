# Configuration file testing
add_test(NAME example 
         COMMAND allpix -c etc/example.conf
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_test(NAME manual 
         COMMAND allpix -c etc/manual.conf
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
         
# Performance and comparison test
add_test(NAME check_performance
         COMMAND allpix -c test/check.conf -l ${CMAKE_BINARY_DIR}/output_check_performance.log
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
