# Define module
ALLPIX_DETECTOR_MODULE(MODULE_NAME)

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} DetectorHistogrammerModule.cpp Cluster.cpp)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
