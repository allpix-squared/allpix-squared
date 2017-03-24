# include the standard dependencies
INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})

# define the library
ADD_LIBRARY(AllpixMessages SHARED
    DepositionMessage.cpp
)

# link the standard dependencies
TARGET_LINK_LIBRARIES(AllpixMessages ${ALLPIX_LIBRARIES} ${ALLPIX_DEPS_LIBRARIES})

# specify install for the messages
INSTALL(TARGETS AllpixMessages
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
