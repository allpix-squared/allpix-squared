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
    config/Configuration.cpp
    AllPix.cpp
)
