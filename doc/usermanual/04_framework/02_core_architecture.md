---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Architecture of the Core"
weight: 2
---

The core is constructed as a light-weight framework which provides various subsystems to the modules. It contains the part of
the software responsible for instantiating and running the modules from the supplied configuration file, and is structured
around five subsystems, of which four are centered around a manager and the fifth contains a set of general utilities. The
systems provided are:

1. **Configuration**:
   The configuration subsystem provides a configuration object from which data can be retrieved or stored, together with a
   TOML-like \[[@tomlgit]\] parser to read configuration files. It also contains the Allpix Squared configuration manager
   which provides access to the main configuration file and its sections. It is used by the module manager system to find
   the required instantiations and access the global configuration. More information is given in
   [Section 4.3](./03_configuration.md).

2. **Module**:
   The module subsystem contains the base class of all Allpix Squared modules as well as the manager responsible for loading
   and executing the modules (using the configuration system). This component is discussed in more detail in
   [Section 4.4](./04_modules.md).

3. **Geometry**:
   The geometry subsystem supplies helpers for the simulation geometry. The manager instantiates all detectors from the
   detector configuration file. A detector object contains the position and orientation linked to an instantiation of a
   particular detector model, itself containing all parameters describing the geometry of the detector. More details about
   geometry and detector models is provided in [Chapter 5](../05_geometry_detectors/_index.md).

4. **Messenger**:
   The messenger is responsible for sending objects from one module to another. The messenger object is passed to every
   module and can be used to bind to messages to listen for. Messages with objects are also dispatched through the messenger
   as described in [Section 4.6](./06_messages.md).

5. **Utilities**:
   The framework provides a set of utilities for logging, file and directory access, and unit conversion. An explanation on
   how to use of these utilities can be found in [Section 4.8](./08_logging.md). A set of C++ exceptions is also provided
   in the utilities, which are inherited and extended by the other components. Proper use of exceptions, together with
   logging information and reporting errors, makes the framework easier to use and debug. A few notes about the use and
   structure of exceptions are provided in [Section 4.9](./09_error_reporting.md).

[@tomlgit]: https://github.com/toml-lang/toml
