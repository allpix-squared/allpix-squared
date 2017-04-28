![](doc/logo_small.png)

# allpix<sup>2</sup>
Generic simulation framework for pixel detectors based on [original AllPix](https://github.com/AllPix/allpix).

**WARNING: Allpix<sup>2</sup> is currently in alpha state and any part of the interface is subject to change**

[![build status](https://gitlab.cern.ch/simonspa/allpix-squared/badges/master/build.svg)](https://gitlab.cern.ch/simonspa/allpix-squared/commits/master)
[![coverity status](https://scan.coverity.com/projects/11975/badge.svg)](https://scan.coverity.com/projects/koensw-allpix-squared)

## Dependencies
* [ROOT](https://root.cern.ch/building-root) with GenVector
* [Geant4](http://geant4.web.cern.ch/geant4/UserDocumentation/UsersGuides/InstallationGuide/html/ch02.html) (optional in a later stage)
* [Eigen3](http://eigen.tuxfamily.org/index.php?title=Main_Page) (optional in a later stage)

## Compilation/Installation
The standard CMake build system is used. The install directory can be specified by adding -DCMAKE_INSTALL_PREFIX=<prefix> as argument to the cmake command below. More configuration options will be added later. Make sure to load your ROOT and Geant4 setups for the build to succeed. To compile and install run the following commands:

```
$ mkdir build && cd build/
$ cmake ..
$ make
$ make install
```

## Documentation
The manual contains a description about the design choices and implementation of allpix<sup>2</sup>. An extensive documentation for users is currently not available, but see below for some notes how to get started. The manual can be built from the LaTeX sources via
```
$ make pdf
```

Doxygen reference is currently incomplete, but can be build with
```
$ make reference
```

## Using the framework
#### Setting up the configuration
Before running AllPix<sup>2</sup> a configuration needs to be provided for both the modules and the detector setup. Also several files needs to be provided that contain the description of the detector models. The configuration format is a simple version of the [TOML](https://github.com/toml-lang/toml) format (quite similar to .ini files).

An example for the main configuration can be found in `etc/example_config.ini`. The configuration consist of several sections that describe the modules to instantiate and the configuration to use for those (more details about the instantiation logic can be found in the manual). The available modules are found in the *src/modules/* directory (note that modules can be added / change and removed at any moment in the current development stage). The path to any of the files in the configuration should either be absolute paths or relative to the location of the configuration file.

Currently a simple test model is provided in the *models/* directory as `models/test.ini` together with a TimePix chip at `models/timepix.ini`. An example setup for the detectors (type, position and orientation) can be found in `etc/example_detector_config.ini`. The specified detector types should exist in the list of models (see the user manual for details about the search path for models).

The test visualization module can be uncommented to display the result in Geant4 using the provided [driver](http://geant4.cern.ch/G4UsersDocuments/UsersGuides/ForApplicationDeveloper/html/Visualization/visdrivers.html) (the 'VRML2FILE' driver that visualizes afterwards with a viewer like [freewrl](http://freewrl.sourceforge.net/) is currently the recommended option). The supplied init and run macros should normally not be changed and point to the ones provided in the *etc* folder.

#### Running simulations
The executable can be found in `bin/allpix` after installation (in the supplied installation prefix, defaulting to the source directory in a standard build). The AllPix libraries are found in the `lib/` directory. To run the simulation simply execute the following (replacing `<your_config_file>` with your own configuration; take `etc/example_config.ini` as a starting point)
```
$ bin/allpix <your_config_file>
```

#### Adding your own module
The framework supports the development of custom modules. Note however that the module definition, the communication with the core framework as well as the communication between the modules are under development and can currently change without notice.

Before starting with the development of your own module, please have a look at the following material:
* the user manual for an introduction to the framework structure (see above how to build).
* (optional) the separate [design document](DESIGN.md) for some more in depth notes about the design evolution
* (optional) some design figures and setup structure in the *doc/design* directory

An example module to start from is found in the directory *src/modules/Example*. There is also a dummy module serving as start for every new module, which can be found in *src/modules/Dummy*. This module also contains a description about the minimum work necessary to get your own module running in the framework. All the other modules in the *src/modules/* directory can also be viewed as examples for doing more complicated tasks.
