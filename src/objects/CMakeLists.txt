# include the standard dependencies
INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})

# Load the generate dictionaries macro
INCLUDE(${ROOT_USE_FILE}/../modules/RootNewMacros.cmake)

# Generate the ROOT dictionary
ROOT_GENERATE_DICTIONARY(AllpixObjectsDictionary
    ${CMAKE_CURRENT_SOURCE_DIR}/Object.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SensorCharge.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PropagatedCharge.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DepositedCharge.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PixelCharge.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PixelHit.hpp
    LINKDEF 
    ${CMAKE_CURRENT_SOURCE_DIR}/Linkdef.h
    MODULE
    AllpixObjects
)

FOREACH(dir IN ITEMS ${ALLPIX_DEPS_INCLUDE_DIRS})
    IF(NOT INCLUDE_DIR_ARGS)
        SET(INCLUDE_DIR_ARGS "-I${dir}")
    ELSE() 
        SET(INCLUDE_DIR_ARGS "${INCLUDE_DIR_ARGS} -I${dir}")
    ENDIF()
ENDFOREACH()

# Compile the dictionary through a special target 
# WARNING: this hack is necessary to prevent standard warnings and those in clang-tidy
ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/AllpixObjectsDictionary.cxx.o COMMAND ${CMAKE_CXX_COMPILER} -fPIC -std=c++${CMAKE_CXX_STANDARD} -I${CMAKE_SOURCE_DIR}/src ${INCLUDE_DIR_ARGS} -o ${CMAKE_CURRENT_BINARY_DIR}/AllpixObjectsDictionary.cxx.o -c ${CMAKE_CURRENT_BINARY_DIR}/AllpixObjectsDictionary.cxx DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/AllpixObjectsDictionary.cxx)

# Define the library adding the object file created above
ADD_LIBRARY(AllpixObjects SHARED
    Object.cpp
    SensorCharge.cpp
    PixelCharge.cpp
    DepositedCharge.cpp
    PropagatedCharge.cpp
    PixelHit.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AllpixObjectsDictionary.cxx.o
)

# Link the standard dependencies
TARGET_LINK_LIBRARIES(AllpixObjects ${ALLPIX_DEPS_LIBRARIES})

# Specify install for the messages
INSTALL(TARGETS AllpixObjects
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)
  
# Also install the dictionary objects
INSTALL(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/libAllpixObjects_rdict.pcm
    ${CMAKE_CURRENT_BINARY_DIR}/libAllpixObjects.rootmap
    DESTINATION lib)

