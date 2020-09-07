[![](doc/logo_small.png)](https://cern.ch/allpix-squared)

# Allpix<sup>2</sup>
### Generic Pixel Detector Simulation Framework

For more details about the project please have a look at the website at https://cern.ch/allpix-squared.

[![build status](https://gitlab.cern.ch/allpix-squared/allpix-squared/badges/master/build.svg)](https://gitlab.cern.ch/allpix-squared/allpix-squared/commits/master)
[![coverity status](https://scan.coverity.com/projects/11975/badge.svg)](https://scan.coverity.com/projects/koensw-allpix-squared)

## Dependencies
* [ROOT](https://root.cern.ch/building-root) (required, with the GenVector component)
* [Geant4](http://geant4-userdoc.web.cern.ch/geant4-userdoc/UsersGuides/InstallationGuide/html/installguide.html) (optional, but required for typical purposes)
* [Eigen3](http://eigen.tuxfamily.org/index.php?title=Main_Page) (optional, but required for typical purposes)

## Usage on CERN machines

Ready-to-run installations of Allpix<sup>2</sup> are provided via the CERN Virtual Machine File System (CVMFS) for use on SLC6 and CentOS7 machines supported by CERN. In order to use Allpix<sup>2</sup>, the respective environment has to be set up by sourcing the appropriate file:

```
$ source /cvmfs/clicdp.cern.ch/software/allpix-squared/<version>/x86_64-<system>-<compiler>-opt/setup.sh
```
where `<version>` should be replaced with the desired Allpix<sup>2</sup> version and `<system>` with the operating system of the executing machine (either `slc6` or `centos7`). The compiler versions available via the `<compiler>` tag depend on the selected operating system.

After this, the `allpix` executable is in the `$PATH` environment variable and can be used.
It should be noted that the CVMFS cache of the executing machine has to be populated with all dependencies when running the program for the first time.
This can lead to a significant start-up time for the first execution, but should not affect further executions with the cache already present.
More information can be found in the [CVMFS documentation](https://cernvm.cern.ch/portal/filesystem).

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
The Doxygen reference is published [online](https://cern.ch/allpix-squared/reference/) too.

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
* Daniel Hynds, Nikhef, @dhynds
* Paul Schütze, DESY, @pschutze
* Simon Spannagel, DESY, @simonspa

The following authors, in alphabetical order, have contributed to Allpix<sup>2</sup>:
* Mohamed Moanis Ali, Free University of Bozen-Bolzano, @mmoanis
* Mathieu Benoit, BNL, @mbenoit
* Thomas Billoud, Université de Montréal, @tbilloud
* Tobias Bisanz, CERN, @tbisanz
* Koen van den Brandt, Nikhef, @kvandenb
* Liejian Chen, Institute of High Energy Physics Beijing, @chenlj
* Katharina Dort, University of Gie\ss en, @kdort
* Neal Gauvin, Université de Genève, @ngauvin
* Maoqiang Jing, University of South China, Institute of High Energy Physics Beijing, @mjing
* Moritz Kiehn, Université de Genève, @msmk
* Salman Maqbool, CERN Summer Student, @smaqbool
* Sebastien Murphy, ETHZ, @smurphy
* Andreas Matthias Nürnberg, KIT, @nurnberg
* Sebastian Pape, TU Dortmund University, @spape
* Marko Petric, CERN, @mpetric
* Nashad Rahman, The Ohio State University, @nashadroid
* Edoardo Rossi, DESY, @edrossi
* Andre Sailer, CERN, @sailer
* Enrico Jr. Schioppa, Unisalento and INFN Lecce, @schioppa
* Sanchit Sharma, Kansas State University, @SanchitKratos
* Xin Shi, Institute of High Energy Physics Beijing, @xshi
* Ondrej Theiner, Charles University, @otheiner
* Mateus Vicente Barreto Pinto, CERN, @mvicente
* Andy Wharton, Lancaster University, @awharton
* Morag Williams, University of Glasgow, @williamm

The authors would also like to express their thanks to the developers of [AllPix](https://twiki.cern.ch/twiki/bin/view/Main/AllPix).

## Citations
The reference paper of Allpix Squared describing the framework and providing first validation results using test beam data has been published in *Nuclear Instrumentations and Methods in Physics Research A*.
The paper is published with open access and can be obtained from here:

https://doi.org/10.1016/j.nima.2018.06.020

Please cite this paper when publishing your work using Allpix Squared as:

> S. Spannagel et al., “Allpix<sup>2</sup>: A modular simulation framework for silicon detectors”, Nucl. Instr.
> Meth. A 901 (2018) 164 – 172, doi:10.1016/j.nima.2018.06.020, arXiv:1806.05813

A preprint version is available on [arxiv.org](https://arxiv.org/abs/1806.05813).

In addition, the software can be cited using the [versioned Zenodo record](https://doi.org/10.5281/zenodo.3550935) or the current version as:

> S. Spannagel, K. Wolters, D. Hynds. (2019, July 9). Allpix Squared - Generic Pixel Detector Simulation Framework
> (Version 1.4.0). Zenodo. http://doi.org/10.5281/zenodo.3550971

## Contributing
All types of contributions, being it minor and major, are very welcome. Please refer to our [contribution guidelines](CONTRIBUTING.md) for a description on how to get started.

Before adding changes it is very much recommended to carefully read through the documentation in the User Manual first.

## Licenses
This software is distributed under the terms of the MIT license. A copy of this license can be found in [LICENSE.md](LICENSE.md).

The documentation is distributed under the terms of the CC-BY-4.0 license. This license can be found in [doc/COPYING.md](doc/COPYING.md).

The LaTeX, Pandoc and PostgreSQL CMake modules used by Allpix<sup>2</sup> are licensed under the BSD 3-Clause License.

The octree library of the TCAD DF-ISE converter is made available under the MIT license, more information [here](tools/tcad_dfise_converter/README.md).

The cereal C++11 serialization library used by Allpix<sup>2</sup> is published under the BSD 3-Clause License, the original source code can be found [here](https://github.com/USCiLab/cereal).

The combination algorithms by Howard Hinnant are published under the Boost Software License Version 1.0, the code can be found [here](https://github.com/HowardHinnant/combinations) and the documentation of the class is published [here](https://howardhinnant.github.io/combinations/combinations.html).
