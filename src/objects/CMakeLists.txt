# include the standard dependencies
INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})
INCLUDE(${ROOT_USE_FILE}/../modules/RootNewMacros.cmake)

#MESSAGE(${ROOT_USE_FILE})
#INCLUDE(${ROOT_USE_FILE})
ROOT_GENERATE_DICTIONARY(dictionary_objects 
    Object.hpp
    SensorCharge.hpp
    PropagatedCharge.hpp
    DepositedCharge.hpp
    PixelCharge.hpp
    PixelHit.hpp
    LINKDEF 
    Linkdef.h
)

SET_SOURCE_FILES_PROPERTIES(dictionary_objects.cxx COMPILE_FLAGS "-Wno-old-style-cast -Wno-missing-declarations -Wno-zero-as-null-pointer-constant -Wno-sign-conversion -Wno-error")

# define the library
ADD_LIBRARY(AllpixObjects SHARED
    dictionary_objects.cxx
    Object.cpp
    SensorCharge.cpp
    PixelCharge.cpp
    DepositedCharge.cpp
    PropagatedCharge.cpp
    PixelHit.cpp
)

# link the standard dependencies
TARGET_LINK_LIBRARIES(AllpixObjects ${ALLPIX_DEPS_LIBRARIES})

# specify install for the messages
INSTALL(TARGETS AllpixObjects
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
