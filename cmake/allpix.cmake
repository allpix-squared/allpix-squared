# for every module, build a separate library to be loaded by allpix core
MACRO(allpix_build_module dir)
    # FIXME: always build all modules by default for now
    OPTION(BUILD_${dir} "Build module in directory ${dir}?" ON)
    
    # only build if the build flag is defined
    IF(BUILD_${dir} OR BUILD_allmodules)
        MESSAGE( STATUS "Building module: " ${dir} )
    
        # build the module
        ADD_SUBDIRECTORY(${dir})
    ENDIF()
ENDMACRO()

# common module definitions
MACRO(_allpix_module_define_common name)
    # get the name of the module
    GET_FILENAME_COMPONENT(_allpix_module_dir ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    
    # prepend with the allpix module prefix to create the name of the module
    SET(${name} "AllpixModule${_allpix_module_dir}")
    
    # save the module library for prelinking in the executable (NOTE: see exec folder)
    SET(ALLPIX_MODULE_LIBRARIES ${ALLPIX_MODULE_LIBRARIES} ${${name}} CACHE INTERNAL "Module libraries")
    
    # set default module class name
    SET(_allpix_module_class "${_allpix_module_dir}Module")
        
    # find if alternative module class name is passed or we can use the default
    SET (extra_macro_args ${ARGN})
    LIST(LENGTH extra_macro_args num_extra_args)
    IF (${num_extra_args} GREATER 0)
        MESSAGE (AUTHOR_WARNING "Provided non-standard module class name! Naming it ${_allpix_module_class} is recommended")
        LIST(GET extra_macro_args 0 _allpix_module_class)
    ENDIF ()
    
    # check if main header file is defined
    IF(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_allpix_module_class}.hpp")
        MESSAGE(FATAL_ERROR "Header file ${_allpix_module_class}.hpp does not exist, cannot build module! \
Create the header or provide the alternative class name as first argument")
    ENDIF()
    
    # define the library
    ADD_LIBRARY(${${name}} SHARED "")
        
    # add the current directory as include directory
    TARGET_INCLUDE_DIRECTORIES(${${name}} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    
    # set the special header flags and add the special dynamic implementation file
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE ALLPIX_MODULE_NAME=${_allpix_module_class})
    # WARNING: this only works if modules follow the convention by putting Module at the end
    # FIXME: find a better way to force users to do this and inform them with a more useful message
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE ALLPIX_MODULE_HEADER="${_allpix_module_class}.hpp")
    TARGET_SOURCES(${${name}} PRIVATE "${PROJECT_SOURCE_DIR}/src/core/module/dynamic_module_impl.cpp")
ENDMACRO()

# put this at the start of every unique module
MACRO(allpix_unique_module name)
    _allpix_module_define_common(${name} ${ARGN})
    
    # set the unique flag to true
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE ALLPIX_MODULE_UNIQUE=1)
ENDMACRO()

# put this at the start of every detector module
MACRO(allpix_detector_module name)
    _allpix_module_define_common(${name} ${ARGN})
    
    # set the unique flag to false
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE ALLPIX_MODULE_UNIQUE=0)
ENDMACRO()

# add sources to the module
MACRO(allpix_module_sources name)
    # get the list of sources
    SET(_list_var "${ARGN}")
    LIST(REMOVE_ITEM _list_var ${name}) 

    # include directories for dependencies
    INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})
    
    # add the library
    TARGET_SOURCES(${name} PRIVATE ${_list_var})
    
    # link the standard allpix libraries
    TARGET_LINK_LIBRARIES(${name} ${ALLPIX_LIBRARIES} ${ALLPIX_DEPS_LIBRARIES})
ENDMACRO()

# provide default install target for the module
MACRO(allpix_module_install name)
    INSTALL(TARGETS ${name}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
ENDMACRO()
