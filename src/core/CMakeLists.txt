# build with Weffc++ only in the core
IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weffc++")
ENDIF()

# create core library
ADD_LIBRARY(AllpixCore SHARED
    utils/log.cpp
    module/Module.cpp
    module/ModuleManager.cpp
    module/StaticModuleManager.cpp
    module/ModuleFactory.cpp
    messenger/Messenger.cpp
    messenger/Message.cpp
    config/Configuration.cpp
    config/ConfigReader.cpp
    config/ConfigManager.cpp
    geometry/Detector.cpp
    geometry/DetectorModel.cpp
    geometry/GeometryManager.cpp
    AllPix.cpp
)

INSTALL(TARGETS AllpixCore
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
