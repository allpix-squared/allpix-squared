[![](doc/logo_small.png)](https://cern.ch/allpix-squared)

# Allpix<sup>2</sup>
### Generic Pixel Detector Simulation Framework

For more details about the project please have a look at the website at https://cern.ch/allpix-squared.

[![build status](https://gitlab.cern.ch/allpix-squared/allpix-squared/badges/master/build.svg)](https://gitlab.cern.ch/allpix-squared/allpix-squared/commits/master)
[![coverity status](https://scan.coverity.com/projects/11975/badge.svg)](https://scan.coverity.com/projects/koensw-allpix-squared)

## Dependencies
* [ROOT](https://root.cern.ch/building-root) (required, with the GenVector component)
* [Geant4](http://geant4.web.cern.ch/geant4/UserDocumentation/UsersGuides/InstallationGuide/html/ch02.html) (optional, but required for typical purposes)
* [Eigen3](http://eigen.tuxfamily.org/index.php?title=Main_Page) (optional, but required for typical purposes)

## Installation
The CMake build system is used for compilation and installation. The install directory can be specified by adding `-DCMAKE_INSTALL_PREFIX=<prefix>` as argument to the CMake command below. Other configuration options are explained in the manual (see the documentation section below).

The dependencies need to be initialized for the build to succeed. Currently there are two methods to load these:

### Prerequisites on CERN LXPLUS
In order to install Allpix<sup>2</sup> on the CERN LXPLUS batch cluster, a LXPLUS setup script is provided:
```
$ source etc/scripts/setup_lxplus.sh
```
Then, continue as described under the compilation section.

### Prerequisites on a private machine
The dependencies listen above have to be satisfied. Both ROOT6 and Geant4 libraries and headers have to be in the path, this is usually achieved by sourcing the `thisroot.sh` or `geant4.sh` scripts. After this, continue as described below.

### Compilation
To compile and install a default installation of Allpix<sup>2</sup>, run the following commands

```
$ mkdir build && cd build/
$ cmake ..
$ make install
```

For more detailed installation instructions, please refer to the documentation below.

## Documentation
The most recently published version of the User Manual is available online [here](https://cern.ch/allpix-squared/usermanual/allpix-manual.pdf).
The Doxygen reference is published [online](https://cern.ch/allpix-squared/reference/html/) too.

The latest PDF version of the User Manual can also be created from source by executing
```
$ make pdf
```
After running the manual is available under `usermanual/allpix-manual.pdf` in the build directory.

To build the HTML version of the latest Doxygen reference, run the following command
```
$ make reference
```
The main page of the reference can then be found at `reference/html/index.html` in the build folder.

## Development of Allpix<sup>2</sup>

Allpix<sup>2</sup> has been developed and is maintained by

* Koen Wolters, CERN, @kwolters
* Daniel Hynds, CERN, @dhynds
* Simon Spannagel, CERN, @simonspa

The following authors, in alphabetical order, have contributed to Allpix<sup>2</sup>:

* Neal Gauvin, Université de Genève, @ngauvin
* Moritz Kiehn, Université de Genève, @msmk
* Salman Maqbool, CERN Summer Student, @smaqbool
* Andreas Matthias Nürnberg, CERN, @nurnberg
* Marko Petric, CERN, @mpetric
* Edoardo Rossi, DESY, @edrossi
* Andre Sailer, CERN, @sailer
* Paul Schütze, DESY, @pschutze
* Mateus Vicente Barreto Pinto, Université de Genève, @mvicente

The authors would also like to express their thanks to the developers of [AllPix](https://twiki.cern.ch/twiki/bin/view/Main/AllPix).

## Contributing
All types of contributions, being it minor and major, are very welcome. Please refer to our [contribution guidelines](CONTRIBUTING.md) for a description on how to get started.

Before adding changes it is very much recommended to carefully read through the documentation in the User Manual first.

## Licenses
This software is distributed under the terms of the MIT license. A copy of this license can be found in [LICENSE.md](LICENSE.md).

The documentation is distributed under the terms of the CC-BY-4.0 license. This license can be found in [doc/COPYING.md](doc/COPYING.md).

The LaTeX and Pandoc CMake modules used by Allpix<sup>2</sup> are licensed under the BSD 3-Clause License.

The octree library of the TCAD DF-ISE converter is made available under the MIT license, more information [here](tools/tcad_dfise_converter/README.md).
