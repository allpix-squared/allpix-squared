---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Building Modules Outside the Framework"
weight: 2
---

Allpix Squared provides CMake modules which allow to build modules for the framework outside the actual code repository. The
macros required to build a module are provided through the CMake modules and are automatically included when using the
`FIND_PACKAGE(Allpix)` CMake command. By this, modules can easily be moved into and out from the module directory of the
framework without requiring changes to its `CMakeLists.txt`.

A minimal CMake setup for compiling and linking external modules to the core and object library of the Allpix Squared
framework is the following:

```cmake
CMAKE_MINIMUM_REQUIRED(VERSION 3.6.3 FATAL_ERROR)

FIND_PACKAGE(Allpix 2.2 REQUIRED)

ALLPIX_DETECTOR_MODULE(MODULE_NAME)
ALLPIX_MODULE_SOURCES(${MODULE_NAME} MySimulationModule.cpp)
```

All dependencies of the framework such as ROOT or Boost.Random are automatically added as CMake targets and can be used by
the module. The required `CMAKE_CXX_STANDARD` is automatically inferred from the settings used to build the framework.
Additional libraries can be linked to the module using the standard CMake command
```cmake
TARGET_LINK_LIBRARIES(${MODULE_NAME} MyExternalLibrary)
```

A more complex CMake structure, suited to host multiple external modules, is provided in a separate repository
\[[@ap2-external-modules]\].

In order to load modules which have been compiled and installed in a different location than the ones shipped with the
framework itself, the respective search path has to be configured properly in the Allpix Squared main configuration file:

```ini
[AllPix]
# Library search paths
library_directories = "~/allpix-modules/build", "/opt/apsq-modules"
```

The relevant parameter is described in detail in [Section 3.4](../03_getting_started/04_framework_parameters.md).


[@ap2-external-modules]: https://gitlab.cern.ch/allpix-squared/external-modules/
