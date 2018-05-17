# Define module
ALLPIX_DETECTOR_MODULE(MODULE_NAME)

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} 
    GenericPropagationModule.cpp
)

# Eigen is required for Runge-Kutta propagation
FIND_PACKAGE(Eigen3 REQUIRED)
TARGET_LINK_LIBRARIES(${MODULE_NAME} Eigen3::Eigen)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
