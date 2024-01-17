---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Configuration via CMake"
weight: 5
---

Allpix Squared uses the CMake build system to configure, build and install the core framework as well as all modules. An
out-of-source build is recommended: this means CMake should not be directly executed in the source folder. Instead, a build
folder should be created, from which CMake should be run. For a standard build without any additional flags this implies
executing:

```shell
mkdir build
cd build
cmake ..
```

CMake can be run with several extra arguments to change the type of installation. These options can be set with `-D<option>`
(see the end of this section for an example). Currently the following options are supported:

- `CMAKE_INSTALL_PREFIX`:
  The directory to use as a prefix for installing the binaries, libraries and data. Defaults to the source directory (where
  the folders `bin/` and `lib/` are added).

- `CMAKE_BUILD_TYPE`:
  Type of build to install, defaults to `RelWithDebInfo` (compiles with optimizations and debug symbols). Other possible
  options are `Debug` (for compiling with no optimizations, but with debug symbols and extended tracing using the Clang
  Address Sanitizer library) and `Release` (for compiling with full optimizations and no debug symbols).

- `MODEL_DIRECTORY`:
  Directory to install the internal models to. Defaults to not installing if the `CMAKE_INSTALL_PREFIX` is set to the
  directory containing the sources (the default). Otherwise the default value is equal to the directory
  `<CMAKE_INSTALL_PREFIX>/share/allpix/`. The install directory is automatically added to the model search path used by the
  geometry model parsers to find all of the detector models.

- `BUILD_TOOLS`:
  Enable or disable the compilation of additional tools such as the mesh converter. Defaults to `ON`.

- `BUILD_<ModuleName>`:
  If the specific module should be installed or not. Defaults to `ON` for most modules, however some modules with large
  additional dependencies such as LCIO \[[@lcio]\] are disabled by default. This set of parameters allows to configure the
  build for minimal requirements.

- `BUILD_ALL_MODULES`:
  Build all included modules, defaulting to `OFF`. This overwrites any selection using the parameters described above.

An example of a custom debug build, without the [`GeometryBuilderGeant4` module](../08_modules/geometrybuildergeant4.md) and
with installation to a custom directory is shown below:

```shell
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=../install/ \
      -DCMAKE_BUILD_TYPE=DEBUG \
      -DBUILD_GeometryBuilderGeant4=OFF ..
```


[@lcio]: https://doi.org/10.1109/NSSMIC.2012.6551478
