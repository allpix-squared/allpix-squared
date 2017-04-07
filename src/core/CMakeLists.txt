# build with Weffc++ only in the core
IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weffc++")
ENDIF()

# include the dependencies
INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})

# create core library 
ADD_LIBRARY(AllpixCore SHARED
    utils/log.cpp
    utils/unit.cpp
    module/Module.cpp
    module/ModuleManager.cpp
    module/StaticModuleManager.cpp
    module/ModuleFactory.cpp
    messenger/Messenger.cpp
    messenger/BaseMessage.cpp
    config/Configuration.cpp
    config/ConfigReader.cpp
    config/ConfigManager.cpp
    geometry/Detector.cpp
    geometry/DetectorModel.cpp
    geometry/GeometryManager.cpp
    AllPix.cpp
)

# link the dependencies
TARGET_LINK_LIBRARIES(AllpixCore ${ALLPIX_DEPS_LIBRARIES})

# creat standard install target
INSTALL(TARGETS AllpixCore
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
