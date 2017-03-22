# For every module, build a separate library to be loaded by allpix core
MACRO(allpix_build_module dir)
    # FIXME: always build all modules by default for now
    OPTION(BUILD_${dir} "Build module in directory ${dir}?" ON)

    # FIXME: temporarily save all module libraries until we have dynamic loading
    SET(ALLPIX_MODULE_LIBRARIES ${ALLPIX_MODULE_LIBRARIES} "AllpixModule${dir}" CACHE INTERNAL "ALLPIX_MODULE_LIBRARIES")
    
    # only build if the build flag is defined
    IF(BUILD_${dir} OR BUILD_allmodules)
    MESSAGE( STATUS "Building module: " ${dir} )
    ADD_SUBDIRECTORY(${dir})
    ENDIF()
ENDMACRO()

# put this at the start of every module to initialize the name
MACRO(allpix_module name)
    # get the name of the module
    GET_FILENAME_COMPONENT(${name} ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    
    # prepend with the allpix module prefix
    SET(${name} "AllpixModule${${name}}")
    
    # add the current directory as include directory
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
ENDMACRO()

# add sources to the module
MACRO(allpix_module_sources name)
    # get the list of sources
    SET(_list_var "${ARGN}")
    LIST(REMOVE_ITEM _list_var ${name}) 

    # include directories for dependencies
    INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})
    
    # add the library
    ADD_LIBRARY(${name} SHARED ${_list_var})
    
    # link the standard allpix libraries
    TARGET_LINK_LIBRARIES(${name} ${ALLPIX_LIBRARIES} ${ALLPIX_DEPS_LIBRARIES})
ENDMACRO()

# provide default install target for the module
MACRO(allpix_module_install name)
    INSTALL(TARGETS ${ALLPIX_MODULE_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
ENDMACRO()
