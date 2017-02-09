# For every module, build a separate library to be loaded by allpix core

SET(LIB_SOURCE_FILES
  # all algorithms
  "DummyModule.cpp"
  )

ADD_LIBRARY(Algorithms SHARED ${LIB_SOURCE_FILES})
# Link necessary libraries
TARGET_LINK_LIBRARIES(Algorithms ${PROJECT_NAME})
#TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${Geant4_LIBRARIES} ${ROOT_LIBRARIES})

INSTALL(TARGETS Algorithms
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
