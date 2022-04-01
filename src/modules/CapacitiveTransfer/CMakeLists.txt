# Define module and return the generated name as MODULE_NAME
ALLPIX_DETECTOR_MODULE(MODULE_NAME)

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} CapacitiveTransferModule.cpp)

# Eigen is required for finding the coupling distances
FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(Eigen3 REQUIRED IMPORTED_TARGET eigen3)

TARGET_LINK_LIBRARIES(${MODULE_NAME} PkgConfig::Eigen3)

# Register module tests
ALLPIX_MODULE_TESTS(${MODULE_NAME} "tests")

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
