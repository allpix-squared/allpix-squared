# Define module and return the generated name as MODULE_NAME
ALLPIX_DETECTOR_MODULE(MODULE_NAME)

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} 
    CapacitiveTransferModule.cpp
    # ADD SOURCE FILES HERE...
)

# Eigen is required for finding the coupling distances
FIND_PACKAGE(Eigen3 REQUIRED)
TARGET_LINK_LIBRARIES(${MODULE_NAME} Eigen3::Eigen)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
