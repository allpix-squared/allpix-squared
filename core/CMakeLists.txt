# Add source files for the allpix core library

INCLUDE_DIRECTORIES(allpix utils)

SET(LIB_SOURCE_FILES
  # core library
  "allpix/allpix.cc"
  # utilities
  "utils/configuration.cc"
  "utils/utils.cc"
  )

ADD_LIBRARY(${PROJECT_NAME} SHARED ${LIB_SOURCE_FILES})
# Link necessary libraries
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${Geant4_LIBRARIES})

INSTALL(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
