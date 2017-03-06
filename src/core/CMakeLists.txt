# Include directories
#INCLUDE_DIRECTORIES(
#  core/utils
#  core/allpix
#  core/objs
#  core/detector
#  modules
#  ${GEANT4_INCLUDE_DIR})

#FIND_PACKAGE( PkgConfig )
FIND_PACKAGE( EIGEN3 REQUIRED eigen3 )
INCLUDE_DIRECTORIES(SYSTEM ${EIGEN3_INCLUDE_DIRS} )

SET (CMAKE_CXX_STANDARD 14)

ADD_LIBRARY(AllpixCore SHARED
    utils/log.cpp
#    utils/string.cpp
    module/Module.cpp
    module/ModuleManager.cpp
    module/StaticModuleManager.cpp
    module/ModuleFactory.cpp
    messenger/Messenger.cpp
    config/Configuration.cpp
    config/SimpleConfigManager.cpp
    geometry/Detector.cpp
    geometry/DetectorModel.cpp
    geometry/GeometryManager.cpp
    AllPix.cpp
)

INSTALL(TARGETS AllpixCore
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
