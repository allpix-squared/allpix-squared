# Option to build all modules
OPTION(BUILD_ALL_MODULES "Build all modules?" OFF)

# reset the saved libraries
SET(ALLPIX_MODULE_LIBRARIES "" CACHE INTERNAL "Module libraries")

# Build all the modules 
FILE(GLOB subdirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
FOREACH(subdir ${subdirs})
    IF(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
        # Internally checks if the module is enabled
        ALLPIX_BUILD_MODULE(${subdir})
    ENDIF()
ENDFOREACH()
