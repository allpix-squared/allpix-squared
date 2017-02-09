# For every module, build a separate library to be loaded by allpix core

macro(option_build_module dir)
  if(${dir} MATCHES "dummy")
    option(BUILD_${dir} "Build module in directory ${dir}?" ON)
  ELSE()
    option(BUILD_${dir} "Build module in directory ${dir}?" OFF)
  ENDIF()

  if(BUILD_${dir} OR BUILD_allmodules)
    #build_module(${dir})
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
  endif()
endmacro()

# Option to build all modules
option(BUILD_allmodules "Build all modules?" OFF)

# Example module implementation - check this directory as a starting point
# for your own module implementation
option_build_module(dummy)
