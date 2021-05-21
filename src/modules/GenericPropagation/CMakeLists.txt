# Define module
ALLPIX_DETECTOR_MODULE(MODULE_NAME)

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} GenericPropagationModule.cpp)

# Register module tests
ALLPIX_MODULE_TESTS(${MODULE_NAME} "tests")

# Eigen is required for Runge-Kutta propagation
FIND_PACKAGE(Eigen3 REQUIRED NO_MODULE)
ALLPIX_SETUP_EIGEN_TARGETS()

TARGET_LINK_LIBRARIES(${MODULE_NAME} ROOT::Graf3d Eigen3::Eigen)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
