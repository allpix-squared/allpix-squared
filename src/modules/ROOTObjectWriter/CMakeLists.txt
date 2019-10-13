# Define module
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME}
    ROOTObjectWriterModule.cpp
)

TARGET_LINK_LIBRARIES(${MODULE_NAME} ROOT::Tree)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
