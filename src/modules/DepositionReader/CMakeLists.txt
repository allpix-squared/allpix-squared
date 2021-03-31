# Define module and return the generated name as MODULE_NAME
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} DepositionReaderModule.cpp)

TARGET_LINK_LIBRARIES(${MODULE_NAME} ROOT::Tree ROOT::TreePlayer)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
