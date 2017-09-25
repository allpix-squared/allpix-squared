# For every module, build a separate library to be loaded by allpix core
MACRO(allpix_enable_default val)
    # Get the name of the module
    GET_FILENAME_COMPONENT(_allpix_module_dir ${CMAKE_CURRENT_SOURCE_DIR} NAME)

    # Build all modules by default if not specified otherwise
    OPTION(BUILD_${_allpix_module_dir} "Build module in directory ${_allpix_module_dir}?" ${val})
ENDMACRO()

# Common module definitions
MACRO(_allpix_module_define_common name)
    # Get the name of the module
    GET_FILENAME_COMPONENT(_allpix_module_dir ${CMAKE_CURRENT_SOURCE_DIR} NAME)

    # Build all modules by default if not specified otherwise
    OPTION(BUILD_${_allpix_module_dir} "Build module in directory ${_allpix_module_dir}?" ON)

    # Quit the file if not building this file or all modules
    IF(NOT (BUILD_${_allpix_module_dir} OR BUILD_ALL_MODULES))
        RETURN()
    ENDIF()

    # Put message
    MESSAGE( STATUS "Building module: " ${_allpix_module_dir} )

    # Prepend with the allpix module prefix to create the name of the module
    SET(${name} "AllpixModule${_allpix_module_dir}")

    # Save the module library for prelinking in the executable (NOTE: see exec folder)
    SET(ALLPIX_MODULE_LIBRARIES ${ALLPIX_MODULE_LIBRARIES} ${${name}} CACHE INTERNAL "Module libraries")

    # Set default module class name
    SET(_allpix_module_class "${_allpix_module_dir}Module")

    # Find if alternative module class name is passed or we can use the default
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

    # Define the library
    ADD_LIBRARY(${${name}} SHARED "")

    # Add the current directory as include directory
    TARGET_INCLUDE_DIRECTORIES(${${name}} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    # Set the special header flags and add the special dynamic implementation file
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE ALLPIX_MODULE_NAME=${_allpix_module_class})
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE ALLPIX_MODULE_HEADER="${_allpix_module_class}.hpp")

    TARGET_SOURCES(${${name}} PRIVATE "${PROJECT_SOURCE_DIR}/src/core/module/dynamic_module_impl.cpp")
    SET_PROPERTY(SOURCE "${PROJECT_SOURCE_DIR}/src/core/module/dynamic_module_impl.cpp" APPEND PROPERTY OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_allpix_module_class}.hpp")
    SET_PROPERTY(SOURCE "${PROJECT_SOURCE_DIR}/src/core/module/Module.cpp" APPEND PROPERTY OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_allpix_module_class}.hpp")
ENDMACRO()

# Put this at the start of every unique module
MACRO(allpix_unique_module name)
    _allpix_module_define_common(${name} ${ARGN})

    # Set the unique flag to true
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE ALLPIX_MODULE_UNIQUE=1)
ENDMACRO()

# Put this at the start of every detector module
MACRO(allpix_detector_module name)
    _allpix_module_define_common(${name} ${ARGN})

    # Set the unique flag to false
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE ALLPIX_MODULE_UNIQUE=0)
ENDMACRO()

# Add sources to the module
MACRO(allpix_module_sources name)
    # Get the list of sources
    SET(_list_var "${ARGN}")
    LIST(REMOVE_ITEM _list_var ${name})

    # Include directories for dependencies
    INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})

    # Add the library
    TARGET_SOURCES(${name} PRIVATE ${_list_var})

    # Link the standard allpix libraries
    TARGET_LINK_LIBRARIES(${name} ${ALLPIX_LIBRARIES} ${ALLPIX_DEPS_LIBRARIES})
ENDMACRO()

# Provide default install target for the module
MACRO(allpix_module_install name)
    INSTALL(TARGETS ${name}
        COMPONENT modules
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
ENDMACRO()
