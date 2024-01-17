# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Define module
ALLPIX_DETECTOR_MODULE(MODULE_NAME)

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} GenericPropagationModule.cpp)

# Register module tests
ALLPIX_MODULE_TESTS(${MODULE_NAME} "tests")

# Eigen is required for Runge-Kutta propagation
PKG_CHECK_MODULES(Eigen3 REQUIRED IMPORTED_TARGET eigen3)

TARGET_LINK_LIBRARIES(${MODULE_NAME} ROOT::Graf3d PkgConfig::Eigen3)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
