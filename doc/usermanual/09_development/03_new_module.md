---
title: "Implementing a New Module"
weight: 3
---

Owing to its modular structure, the functionality of the Allpix Squared
can easily be extended by adding additional modules which can be placed
in the simulation chain. Since the framework serves a wide community,
modules should be as generic as possible, i.e. not only serve the
simulation of a single detector prototype but implement the necessary
algorithms such that they are re-usable for other applications.
Furthermore, it may be beneficial to split up modules to support the
modular design of Allpix Squared.

Before starting the development of a new module, it is essential to
carefully read the documentation of the framework module manager. The
basic steps to implement a new module, hereafter referred to as
`ModuleName`, are the following:

Additional sources of documentation which may be useful during the
development of a module include:

  - The framework documentation for an introduction to the different
    parts of the framework.

  - The module documentation in for a description of the functionality
    of other modules already implemented, and to look for similar modules
    which can help during development.

  - The Doxygen (core) reference documentation included in the
    framework \[[@ap2-doxygen]\].

  - The latest version of the source code of all modules and the Allpix
    Squared core itself.

Any module potentially useful for other users should be contributed back
to the main repository after is has been validated. It is strongly
encouraged to send a merge-request through the mechanism provided by the
software repository \[[@ap2-repo]\].

## Files of a Module

Every module directory should at minimum contain the following documents
(with `ModuleName` replaced by the name of the module):

  - `CMakeLists.txt`: The build script to load the dependencies and
    define the source files of the library.

  - `README.md`: Full documentation of the module.

  - `<ModuleName>Module.hpp`: The header file of the module.

  - `<ModuleName>Module.cpp`: The implementation file of the module.

These files are discussed in more detail below. By default, all modules
added to the `src/modules/` directory will be built automatically by
CMake. If a module depends on additional packages which not every user
may have installed, one can consider adding the following line to the
top of the module's `CMakeLists.txt`:
```cmake
ALLPIX_ENABLE_DEFAULT(OFF)
```

### `CMakeLists.txt`

Contains the build description of the module with the following
components:

1.  On the first line either `ALLPIX_DETECTOR_MODULE(MODULE_NAME)` or
    `ALLPIX_UNIQUE_MODULE(MODULE_NAME)` depending on the type of module
    defined. The internal name of the module is automatically saved in
    the variable `${MODULE_NAME}` which should be used as an argument to
    other functions. Another name can be used by overwriting the
    variable content, but in the examples below, `${MODULE_NAME}` is
    used exclusively and is the preferred method of implementation.

2.  The following lines should contain the logic to load possible
    dependencies of the module (below is an example to load Geant4).
    Only ROOT is automatically included and linked to the module.

3.  A line with `ALLPIX_MODULE_SOURCES(${MODULE_NAME} <sources>)`
    defines the module source files. Here, `<sources>` should be replaced
    by a list of all source files relevant to this module.

4.  Possible lines to include additional directories and to link
    libraries for dependencies loaded earlier.

5.  A line with `ALLPIX_MODULE_REQUIRE_GEANT4_INTERFACE(${MODULE_NAME})`
    adds the Geant4 interface library as explained in Section
    geant4_interface.

6.  A line to register the directory with module tests, for example
    `tests` as in `{ALLPIX_MODULE_TESTS(${MODULE_NAME} "tests")`.

7.  A line containing `ALLPIX_MODULE_INSTALL(${MODULE_NAME})` to set up
    the required target for the module to be installed to.

A simple `CMakeLists.txt` for a module named `Test` which requires Geant4
is provided below as an example.

```cmake
# Define module and save name to MODULE_NAME
# Replace by ALLPIX_DETECTOR_MODULE(MODULE_NAME) to define a detector module
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Load Geant4
FIND_PACKAGE(Geant4 REQUIRED)

# Add the sources for this module
ALLPIX_MODULE_SOURCES(${MODULE_NAME}
    TestModule.cpp
)

# Add Geant4 to the include directories
TARGET_INCLUDE_DIRECTORIES(${MODULE_NAME} SYSTEM PRIVATE ${Geant4_INCLUDE_DIRS})

# Allpix Geant4 interface is required for this module
ALLPIX_MODULE_REQUIRE_GEANT4_INTERFACE(${MODULE_NAME})

# Link the Geant4 libraries to the module library
TARGET_LINK_LIBRARIES(${MODULE_NAME} ${Geant4_LIBRARIES})

# Register module tests
ALLPIX_MODULE_TESTS(${MODULE_NAME} "tests")

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
```

### `README.md`

The `README.md` serves as the documentation for the module and should be
written in GitLab Flavored Markdown \[[@markdown]\]. It is automatically
included in the user manual.

The `README.md` should follow the structure indicated in the `README.md`
file of the `DummyModule` in `src/modules/Dummy`, and should contain at
least the following sections:

  - A YAML header with the name of the module (`title`), a short description
    of the module (`description`) the maintainer (`module_maintainer`) and
    status (`module_status`) of the module.
    If the module is working and well-tested, the status of the module
    should be `Functional`. By default, new modules are given the status
    `Immature`. The maintainer should mention the full name of the module
    maintainer, with their email address in parentheses. A minimal header is
    therefore:
    ```yaml
    title: "ModuleName"
    description: "Some short description"
    module_maintainer: "Example Author (<example@example.org>)"
    module_status: "Functional"
    ```
    In addition, the input (`module_input`) and output (`module_output`)
    objects of the module should be given as well.

  - An H2-size section named **Description**, containing a short
    description of the module.

  - An H2-size section named **Parameters**, with all available
    configuration parameters of the module. The parameters should be
    briefly explained in an itemised list with the name of the parameter
    set as an inline code block.

  - An H2-size section with the title **Usage** which should contain at
    least one simple example of a valid configuration for the module.

### `<ModuleName>Module.hpp` and `<ModuleName>Module.cpp`

All modules should consist of both a header file and a source file. In
the header file, the module is defined together with all of its methods.
Brief Doxygen documentation should be added to explain what each method
does. The source file should provide the implementation of every method
and also its more detailed Doxygen documentation. Methods should only be
declared in the header and defined in the source file in order to keep
the interface clean.

## Module structure

All modules must inherit from the `Module` base class, which can be
found in `src/core/module/Module.hpp`. The module base class provides
two base constructors, a few convenient methods and several methods
which the user is required to override. Each module should provide a
constructor using the fixed set of arguments defined by the framework;
this particular constructor is always called during by the module
instantiation logic. These arguments for the constructor differ for
unique and detector modules.

For unique modules, the constructor for a `TestModule` should be:

```cpp
TestModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager): Module(config) {}
```

For detector modules, the first two arguments are the same, but the last
argument is a `std::shared_ptr` to the linked detector. It should always
forward this detector to the base class together with the configuration
object. Thus, the constructor of a detector module is:

```cpp
TestModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector): Module(config, std::move(detector)) {}
```

The pointer to a Messenger can be used to bind variables to either
receive or dispatch messages as explained in the framework chapter. The
constructor should be used to bind required messages, set configuration
defaults and to throw exceptions in case of failures. Unique modules can
access the GeometryManager to fetch all detector descriptions, while
detector modules directly receive a link to their respective detector.

In addition to the constructor, each module can override the following
methods:

  - `initialize()`: Called once per module from the main thread after
    loading and constructing all modules and before starting the event
    loop. This method can for example be used to initialize histograms.

  - `initializeThread()`: Called after global initialization but before
    event processing and gives the possibility to initialize worker
    thread-specific members for modules. If multithreading is used, this
    method is called by each worker thread separately; if the simulation
    is run single-threaded, it is called once by the main thread.

  - `run(Event* event)`: Called for every event in the simulation, with
    a pointer to the current event object as parameter. An exception
    should be thrown for serious errors, otherwise a warning should be
    logged.

  - `finalizeThread()`: Called for each worker thread after processing
    all events in the run. If multithreading is used, this method is
    called by each worker thread separately; if the simulation is run
    single-threaded, it is called once by the main thread.

  - `finalize()`: Called once per module from the main thread after
    processing all events in the run and before destructing the module.
    Typically used to save the output data (like histograms). Any
    exceptions should be thrown from here instead of the destructor.

If necessary, modules can also access the ConfigurationManager directly
in order to obtain configuration information from other module instances
or other modules in the framework using the `getConfigManager()` call.
This allows to retrieve and e.g. store the configuration actually used
for the simulation alongside the data.

If a module should be run using multithreading but requires to execute
its run method in the order of event numbers, for example a module that
writes to an output file, then the module can inherit from the
`SequentialModule` class, without implementing additional functionality.
This will ensure that the run method will receive events one-by-one and
in the correct sequence.


[@ap2-doxygen]: https://cern.ch/allpix-squared/reference/
[@ap2-repo]: https://gitlab.cern.ch/allpix-squared/allpix-squared
[@markdown]: https://docs.gitlab.com/ee/user/markdown.html
