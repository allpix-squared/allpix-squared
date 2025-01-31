// SPDX-FileCopyrightText: 2016-2025 CERN and the Allpix Squared authors
// SPDX-License-Identifier: MIT

#pragma once

// CMake uses config.cmake.h to generate config.h within the build folder.
#ifndef ALLPIX_CONFIG_H
#define ALLPIX_CONFIG_H

#define PACKAGE_NAME "@CMAKE_PROJECT_NAME@"
#define PACKAGE_VERSION "@ALLPIX_LIB_VERSION@"
#define PACKAGE_STRING PACKAGE_NAME " " PACKAGE_VERSION

#endif
