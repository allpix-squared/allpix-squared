# include dependencies
INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})

# create executable and link the libs
ADD_EXECUTABLE(allpix allpix.cpp)
TARGET_LINK_LIBRARIES(allpix ${ALLPIX_LIBRARIES})

# FIXME: should not be needed after we have dynamic linking
TARGET_LINK_LIBRARIES(allpix ${ALLPIX_MODULE_LIBRARIES})

INSTALL(TARGETS allpix
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib)
