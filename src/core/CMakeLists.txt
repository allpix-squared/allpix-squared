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
