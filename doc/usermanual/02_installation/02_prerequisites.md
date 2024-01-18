---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Prerequisites"
weight: 2
---

If the framework is to be compiled and executed on CERN's LXPLUS service, all build dependencies can be loaded automatically
from the CVMFS ﬁle system.

The core framework is compiled separately from the individual modules and Allpix Squared has therefore only two required
external dependencies:

- ROOT 6 \[[@root]\]:
  ROOT is used for histogramming as well as coordinate transformations. In addition, some modules implement I/O using ROOT
  libraries. The latest stable release of ROOT 6 is recommended and older versions, such as ROOT 5.x, are not supported.
  Please refer to \[[@rootinstallation]\] for instructions on how to install ROOT. ROOT has several components of which the
  `GenVector` package is required to run Allpix Squared. This package is included in the default build. ROOT needs to be
  built using C++17, which is accomplished by supplying the CMake flag `-DCMAKE_CXX_STANDARD=17`.

- Boost.Random 1.64.0 or later \[[@boostrandom]\]:
  Random number generator and distribution library of the Boost project, used in order to get cross-platform portable,
  STL-compatible random number distributions. While STL random number generators are portable and guarantee to deliver the
  same random number sequence given the same seed, random distributions are not, and their implementation is
  platform-dependent leading to different simulation results with the same seed. Since the implementation of some random
  distributions (most notably of `boost::normal_distribution`) has changed in the past, a recent version is required.

For some modules, additional dependencies exist. For details about the dependencies and their installation see
[Chapter 8](../08_modules/_index.md). The following dependencies are needed to compile the standard installation:

- Geant4 \[[@geant4]\]:
  Simulates the desired particles and their interactions with matter, depositing charges in the detectors with the help of
  the constructed geometry. See the instructions in \[[@geant4installation]\] for details on how to install the software.
  All Geant4 data sets are required to run the modules successfully, and Geant4 must be built using C++17. For
  multithreading to be possible, this must also be enabled in the Geant4 installation. It is recommended to enable the
  Geant4 Qt extensions to allow visualization of the detector setup and the simulated particle tracks. A recommended set of
  CMake flags to build a Geant4 package suitable for usage with Allpix Squared are:
  ```
  -DGEANT4_INSTALL_DATA=ON
  -DGEANT4_USE_GDML=ON
  -DGEANT4_USE_QT=ON
  -DGEANT4_USE_XM=ON
  -DGEANT4_USE_OPENGL_X11=ON
  -DCMAKE_CXX_STANDARD=17
  -DGEANT4_BUILD_MULTITHREADED=ON
  -DGEANT4_BUILD_BUILTIN_BACKTRACE=OFF
  ```

- Eigen3 \[[@eigen3]\]:
  Vector package used to perform Runge-Kutta integration, used in some of the charge carrier propagation modules. Eigen is
  available in almost all Linux distributions through the package manager. Otherwise it can be easily installed, comprising
  a header-only library.

Extra flags need to be set for building an Allpix Squared installation without these dependencies. Details about these
configuration options are given in the [Section 2.5](./05_cmake_configuration.md).


[@root]: http://root.cern.ch/
[@rootinstallation]: https://root.cern.ch/building-root
[@boostrandom]: https://www.boost.org/doc/libs/1_75_0/doc/html/boost_random/reference.html
[@geant4]: https://doi.org/10.1016/S0168-9002(03)01368-8
[@geant4installation]: https://geant4-userdoc.web.cern.ch/UsersGuides/InstallationGuide/html
[@eigen3]: http://eigen.tuxfamily.org
