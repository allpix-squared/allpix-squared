# Include directories
#INCLUDE_DIRECTORIES(
#  core/utils
#  core/allpix
#  core/objs
#  core/detector
#  modules
#  ${GEANT4_INCLUDE_DIR})

SET(CMAKE_CXX_FLAGS "-g -Wall -Wextra -pedantic ${CMAKE_CXX_FLAGS}")

find_package( PkgConfig )
pkg_check_modules( EIGEN3 REQUIRED eigen3 )
include_directories(SYSTEM ${EIGEN3_INCLUDE_DIRS} )

ADD_LIBRARY(AllpixCore SHARED
    utils/log.cpp
#    utils/string.cpp
    module/Module.cpp
    module/StaticModuleManager.cpp
    module/ModuleFactory.cpp
    messenger/Messenger.cpp
    config/Configuration.cpp
    config/SimpleConfigManager.cpp
    geometry/Detector.cpp
    geometry/GeometryManager.cpp
    AllPix.cpp
)

INSTALL(TARGETS AllpixCore
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
