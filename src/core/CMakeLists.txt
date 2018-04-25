# Include the dependencies
INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})

# Create core library
ADD_LIBRARY(AllpixCore SHARED
    utils/log.cpp
    utils/text.cpp
    utils/unit.cpp
    module/Module.cpp
    module/ModuleManager.cpp
    module/ThreadPool.cpp
    messenger/Messenger.cpp
    messenger/Message.cpp
    config/exceptions.cpp
    config/Configuration.cpp
    config/ConfigReader.cpp
    config/ConfigManager.cpp
    config/OptionParser.cpp
    geometry/Detector.cpp
    geometry/GeometryManager.cpp
    Allpix.cpp
)

# Link the dependencies
TARGET_LINK_LIBRARIES(AllpixCore ${ALLPIX_DEPS_LIBRARIES})
TARGET_LINK_LIBRARIES(AllpixCore ${ALLPIX_LIBRARIES})

# Define compile-time library extension
TARGET_COMPILE_DEFINITIONS(AllpixCore PRIVATE SHARED_LIBRARY_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}")
# Link the DL libraries
TARGET_LINK_LIBRARIES(AllpixCore ${CMAKE_DL_LIBS})

# Create standard install target
INSTALL(TARGETS AllpixCore
    COMPONENT application
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)
