---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Packaging"
weight: 2
---

Allpix Squared comes with a basic configuration to generate tarballs from the compiled binaries using the CPack command. In
order to generate a working tarball from the current Allpix Squared build, the `RPATH` of the executable should not be set,
otherwise the `allpix` binary will not be able to locate the dynamic libraries. If not set, the global `LD_LIBRARY_PATH` is
used to search for the required libraries:

```shell
mkdir build
cd build
cmake -DCMAKE_SKIP_RPATH=ON ..
make package
```

Since the CMake installation path defaults to the project's source directory, certain files are excluded from the default
installation target created by CMake. This includes the detector models in the `models/` directory as well as the additional
tools provided in `tools/root_analysis_macros/` folder. In order to include them in a release tarball produced by CPack, the
installation path should be set to a location different from the project source folder, for example:

```shell
cmake -DCMAKE_INSTALL_PREFIX=/tmp ..
```

The content of the produced tarball can be extracted to any location of the file system, but requires the ROOT6 and Geant4
libraries as well as possibly additional libraries linked by individual at runtime.

For this purpose, a `setup.sh` shell script is automatically generated and added to the tarball. By default, it contains the
ROOT6 path used for the compilation of the binaries. Additional dependencies, either library paths or shell scripts to be
sourced, can be added via CMake for individual modules using the CMake functions described below. The paths stored correspond
to the dependencies used at compile time, it might be necessary to change them manually when deploying on a different
computer.

## `ADD_RUNTIME_DEP(name)`

This CMake command can be used to add a shell script to be sourced to the setup file. The mandatory argument `name` can
either be an absolute path to the corresponding file, or only the file name when located in a search path known to CMake, for
example:

```cmake
# Add "geant4.sh" as runtime dependency for setup.sh file:
ADD_RUNTIME_DEP(geant4.sh)
```

The command uses the `GET_FILENAME_COMPONENT` command of CMake with the `PROGRAM` option. Duplicates are removed from the
list automatically. Each file found will be written to the setup file as

```shell
source <absolute path to the file>
```

## `ADD_RUNTIME_LIB(names)`

This CMake command can be used to add additional libraries to the global search path. The mandatory argument `names` should
be the absolute path of a library or a list of paths, such as:

```cmake
# This module requires the LCIO library:
FIND_PACKAGE(LCIO REQUIRED)
# The FIND routine provides all libraries in the LCIO_LIBRARIES variable:
ADD_RUNTIME_LIB(${LCIO_LIBRARIES})
```

The command uses the `GET_FILENAME_COMPONENT` command of CMake with the `DIRECTORY` option to determine the directory of the
corresponding shared library. Duplicates are removed from the list automatically. Each directory found will be added to the
global library search path by adding the following line to the setup file:

```shell
export LD_LIBRARY_PATH="<library directory>:$LD_LIBRARY_PATH"
```
