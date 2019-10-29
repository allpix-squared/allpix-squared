# Define module
ALLPIX_DETECTOR_MODULE(MODULE_NAME)

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME}
    GenericPropagationModule.cpp
)

# Eigen is required for Runge-Kutta propagation
FIND_PACKAGE(Eigen3 REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM ${EIGEN3_INCLUDE_DIR})

TARGET_LINK_LIBRARIES(${MODULE_NAME} ROOT::Graf3d)
# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
