# Define module
ALLPIX_DETECTOR_MODULE(MODULE_NAME)

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} WeightingPotentialReaderModule.cpp)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
