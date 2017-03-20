![](doc/logo_small.png)

# allpix<sup>2</sup>
Generic simulation framework for pixel detectors based on [original AllPix](https://github.com/AllPix/allpix).
**WARNING: Allpix<sup>2</sup> is currently in alpha state and any part of the interface is subject to change**

## Dependencies
* [ROOT](https://root.cern.ch/building-root) with GenVector
* [Geant4](http://geant4.web.cern.ch/geant4/UserDocumentation/UsersGuides/InstallationGuide/html/ch02.html) (optional in a later stage)
* [Eigen3](http://eigen.tuxfamily.org/index.php?title=Main_Page) (optional in a later stage)

## Compilation/Installation
The standard CMake build system is used. The install directory can be specified by adding -DCMAKE_INSTALL_PREFIX as argument to the cmake command below. More configuration options will be added later. Make sure to load your ROOT and Geant4 setups for the build to succeed. To compile and install run the following commands:

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

An example for the main configuration can be found in `etc/example.ini`. The configuration consist of several sections that describe the modules to instantiate and the configuration to use for those (more details about the instantiation logic can be found in the manual). The available modules are found in the *src/modules/* directory (note that modules can be added / change and removed at any moment in the current development stage). The path to any of the files in the configuration should either be changed to an absolute path or be relative to the working directory where allpix is run from next. This specifically holds for the files with the models (*models_file*) and detector description (*detectors_file*).

Currently only a simple test model is provided in the *models/* directory as `models/test.ini` and any other model is not yet supported (but more models will be added shortly). An example setup for the detectors (type, position and orientation) can be found in `etc/example_detector_config.ini`. It can be copied to another location and adjusted to your own needs. The specified detector types should exist in the supplied model configuration file.

The test visualization module can be uncommented to display the result in Geant4 using the provided [driver](http://geant4.cern.ch/G4UsersDocuments/UsersGuides/ForApplicationDeveloper/html/Visualization/visdrivers.html) (the 'VRML2FILE' driver that visualizes afterwards with a viewer like [freewrl](http://freewrl.sourceforge.net/) is recommended). The supplied init and run macros should normally not be changed and point directly to the ones provided in the *etc* folder.
#### Running simulations
The executable can be found in `bin/allpix` after installation (relative to parent of the build directory in a standard build). The AllPix libraries are found in the `lib/` directory. To run the simulation execute (replacing `<your_config_file>` with your own configuration or use `etc/example_config.ini` as a starting point)
```
$ bin/allpix <your_config_file>
```

#### Adding your own module
The framework supports the development of custom modules. Note however that the module definition, the communication with the core framework as well as the communication between the modules are under development and can currently change without notice.

Before starting with the development of your own module, please have a look at the following material:
* the user manual for an introduction to the framework structure (see above how to build).
* (optional) the separate [design document](DESIGN.md) for some more in depth notes about the design evolution
* (optional) some design figures and setup structure in the *doc/design* directory

An example module to start from is found in the directory *src/modules/example* (this module is also included in the manual). There is also a dummy module serving as start for every new module, which can be found in *src/modules/dummy*. This module also contains a description about the minimum work necessary to get your own module running in the framework. All the other modules in the *src/modules/* directory can also be viewed as examples for doing more complicated tasks.
