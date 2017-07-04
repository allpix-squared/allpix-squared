![](doc/logo_small.png)

# allpix<sup>2</sup>
Generic simulation framework for pixel detectors based on [original AllPix](https://github.com/AllPix/allpix).

[![build status](https://gitlab.cern.ch/simonspa/allpix-squared/badges/master/build.svg)](https://gitlab.cern.ch/simonspa/allpix-squared/commits/master)
[![coverity status](https://scan.coverity.com/projects/11975/badge.svg)](https://scan.coverity.com/projects/koensw-allpix-squared)

## Dependencies
* [ROOT](https://root.cern.ch/building-root) (required, with the GenVector component)
* [Geant4](http://geant4.web.cern.ch/geant4/UserDocumentation/UsersGuides/InstallationGuide/html/ch02.html) (optional, but required for typical purposes)
* [Eigen3](http://eigen.tuxfamily.org/index.php?title=Main_Page) (optional, but required for typical purposes)

## Compilation/Installation
The CMake build system is used for compilation and installation. The install directory can be specified by adding `-DCMAKE_INSTALL_PREFIX=<prefix>` as argument to the CMake command below. Other configuration options are explained in the manual (see the documentation section below). 

The dependencies need to be initialized for the build to succeed. Currently there are two methods to load these:

### Installation on CERN LXPLUS

In order to install allpix<sup>2</sup> on the CERN LXPLUS batch cluster, a LXPLUS setup script is provided:
```
$ source etc/scripts/setup_lxplus.sh
```
Then, continue as described under the compiliation section.

### Installation on a private machine

The dependencies listen above have to be satisfied. Both ROOT6 and Geant4 libraries and headers have to be in the path, this is usually achieved by sourcing the `thisroot.sh` or `geant4.sh` scripts. After this, continue as described below.

### Compilation
To compile and install a default installation of allpix<sup>2</sup>, run the following commands

```
$ mkdir build && cd build/
$ cmake ..
$ make install
```

## Documentation
The User's Manual of allpix<sup>2</sup> can be created from source by executing 
```
$ make pdf
```
After running the manual is available under `usermanual` in the build directory. A recent version of the documentation is also available online [here](https://gitlab.cern.ch/simonspa/allpix-squared/uploads/c94a3d3a4a2ff54ce136fdc5dedb1c12/allpix-manual.pdf). Several parts of the manual (especially the module documentation) are still mising, but most other sections are fairly complete.

Doxygen reference of the core framework is available. The reference for the modules, tools and objects is currently incomplete and will be added later. To build the HTML version of the Doxygen reference, run the following command
```
$ make reference
```
The main page of the reference can then be found at `reference/html/index.html` in the build folder.

## Using the framework
#### Setting up the configuration
Before running AllPix<sup>2</sup> a configuration needs to be provided for both the modules and the detector setup. Also several files needs to be provided that contain the description of the detector models. The configuration format is a simplified version of the [TOML](https://github.com/toml-lang/toml) format (quite similar to .ini files).

An example for the main configuration can be found in `etc/example.conf`. The configuration consist of several sections that describe the modules to instantiate and the configuration to use for those (more details can be found in the manual). The available modules are found in the *src/modules/* directory. 

Currently a test model is provided in the *models/* directory as `models/test.conf` and also a Timepix is available at `models/timepix.conf`. An example setup for the detectors (type, position and orientation) can be found in `etc/example_detector.conf`. The specified detector types should exist in the list of models (again please refer to the manual for more details).

The visualization module can be uncommented to display the resulting Geant4 geometry. There are various options for display, but the default Qt viewer is recommended (as long as it is available in the Geant4 installation).

#### Running simulations
The executable can be found in `bin/allpix` after installation (in the supplied installation prefix, defaulting to the source directory in a standard build). The libraries are found in the `lib/` directory. To run the simulation simply execute the following (replacing `<your_config_file>` with your own configuration; take `etc/example.conf` as a starting point)
```
$ bin/allpix <your_config_file>
```

#### Adding your own module
The framework supports the development of custom modules. For details about developing a new module, please have a look at the User's Manual.

A dummy module can be found in *src/modules/Dummy*. This module can be copied and adjusted for new modules. All the other modules in the *src/modules/* directory can be viewed as examples for doing more sophisiticated tasks.
