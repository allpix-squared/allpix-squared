---
title: "Main Components"
weight: 1
---

The framework consists of the following four main components that together
form Allpix Squared:

1.  **Core**: The core contains the internal logic to initialize the
    modules, provide the geometry, facilitate module communication and
    run the event sequence. The core keeps its dependencies to a minimum
    (it only relies on ROOT) and remains independent from the other
    components as far as possible. It is the main component discussed in
    this section.

2.  **Modules**: A module is a set of methods which is executed as part
    of the simulation chain. Modules are built as separate libraries and
    loaded dynamically on demand by the core. The available modules and
    their parameters are discussed in detail in the modules chapter.

3.  **Objects**: Objects form the data passed between modules using the
    message framework provided by the core. Modules can listen and bind
    to messages with objects they wish to receive. Messages are
    identified by the object type they are carrying, but can also be
    renamed to allow the direction of data to specific modules,
    facilitating more sophisticated simulation setups. Messages are
    intended to be read-only and a copy of the data should be made if a
    module wishes to change the data. All objects are compiled into a
    separate library which is automatically linked to every module.

4.  **Physics**: In many cases, several modules depend on the same
    underlying physics models. These models are separated from the modules
    themselves.

5.  **Tools**: provides a set of header-only 'tools' and a shared
    library that allow access to common logic shared by various modules.
    Examples are the Runge-Kutta solver \[[@fehlberg]\] implemented using
    the Eigen3 library and the set of template specializations for ROOT and
    Geant4 configurations. This set of tools is different from the set of
    core utilities the framework itself provides, which is part of the core.

Finally, Allpix Squared provides an executable which instantiates the core of the
framework, receives and distributes the configuration object and runs
the simulation chain.

The chapter is structured as follows. The next section provides an overview
of the architectural design of the core and describes its interaction with
the rest of the framework. The different subcomponents such as configuration,
modules and messages are discussed in thereafter. The chapter closes with a
description of the available framework tools. Some code will be provided in
the text, but readers not interested may skip the technical details.


[@fehlberg]: https://ntrs.nasa.gov/search.jsp?R=19690021375
