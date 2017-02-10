# For every module, build a separate library to be loaded by allpix core

MACRO(option_build_module dir)
    IF(${dir} MATCHES "dummy")
        OPTION(BUILD_${dir} "Build module in directory ${dir}?" ON)
    ELSE()
        OPTION(BUILD_${dir} "Build module in directory ${dir}?" OFF)
    ENDIF()

  IF(BUILD_${dir} OR BUILD_allmodules)
    MESSAGE( STATUS "Building module: " ${dir} )

    INCLUDE_DIRECTORIES(${dir})
    FILE(GLOB_RECURSE LIB_SOURCE_FILES ${dir} "*.cpp")
    MESSAGE( STATUS " -- sources: " ${LIB_SOURCE_FILES} )

    ADD_LIBRARY(${dir} SHARED ${LIB_SOURCE_FILES})

    # Link necessary libraries
    TARGET_LINK_LIBRARIES(${dir} ${PROJECT_NAME})

    INSTALL(TARGETS ${dir}
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib)
  ENDIF()
ENDMACRO()

# Option to build all modules
OPTION(BUILD_allmodules "Build all modules?" OFF)

# Example module implementation - check this directory as a starting point for your own module implementation
option_build_module(dummy)
