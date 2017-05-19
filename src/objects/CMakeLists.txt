# include the standard dependencies
INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})

# define the library
ADD_LIBRARY(AllpixObjects SHARED
    Object.cpp
    SensorCharge.cpp
    PixelCharge.cpp
    PixelHit.cpp
)

# link the standard dependencies
TARGET_LINK_LIBRARIES(AllpixObjects ${ALLPIX_DEPS_LIBRARIES})

# specify install for the messages
INSTALL(TARGETS AllpixObjects
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
