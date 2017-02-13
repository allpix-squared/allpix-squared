# Include directories
#INCLUDE_DIRECTORIES(
#  core/utils
#  core/allpix
#  core/objs
#  core/detector
#  modules
#  ${GEANT4_INCLUDE_DIR})

SET(CMAKE_CXX_FLAGS "-g -Wall -Wextra -pedantic ${CMAKE_CXX_FLAGS}")

ADD_LIBRARY(allpixcore SHARED
    messenger/Messenger.cpp
    config/Configuration.cpp
    AllPix.cpp
)

INSTALL(TARGETS allpixcore
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
