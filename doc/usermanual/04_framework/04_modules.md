---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Modules and the Module Manager"
weight: 4
---

Allpix Squared is a modular framework and one of the core ideas is to partition functionality in independent modules which
can be inserted or removed as required. These modules are located in the subdirectory `src/modules/` of the repository, with
the name of the directory the unique name of the module. The suggested naming scheme is CamelCase, thus an example module
name would be `GenericPropagation`. There are two different kind of modules which can be defined:

- **Unique**:
  Modules for which a single instance runs, irrespective of the number of detectors.

- **Detector**:
  Modules which are concerned with only a single detector at a time. These are then replicated for all required detectors.

The type of module determines the constructor used, the internal unique name and the supported configuration parameters. More
details about the instantiation logic for the different types of modules are given in the
[next section](#module-instantiation).

## Module instantiation

Modules are dynamically loaded and instantiated by the Module Manager. They are constructed, initialized, executed and
finalized in the linear order in which they are defined in the configuration file; for this reason the configuration file
should follow the order of the real process. For each section in the main configuration file (see
[Section 4.3](./03_configuration.md) for more details), a corresponding library is searched for which contains the module
(the exception being the global framework section). Module libraries are always named following the scheme
`libAllpixModule<ModuleName>`, reflecting the `ModuleName` configured via CMake. The module search order is as follows:

1. Modules already loaded before from an earlier section header

2. All directories in the global configuration parameter `library_directories` in the provided order, if this parameter
   exists.

3. The internal library paths of the executable, that should automatically point to the libraries that are built and
   installed together with the executable. These library paths are stored in `RPATH` on Linux, see the next point for more
   information.

4. The other standard locations to search for libraries depending on the operating system. Details about the procedure Linux
   follows can be found in \[[@linuxld]\].

If the loading of the module library is successful, the module is checked to determine if it is a unique or detector module.
As a single module may be called multiple times in the configuration, with overlapping requirements (such as a module which
runs on all detectors of a given type, followed by the same module but with different parameters for one specific detector,
also of this type) the Module Manager must establish which instantiations to keep and which to discard. The instantiation
logic determines a unique name and priority, where a lower number indicates a higher priority, for every instantiation. The
name and priority for the instantiation are determined differently for the two types of modules:

- **Unique**:
  Combination of the name of the module and the `input` and `output` parameter (both defaulting to an empty string). The
  priority is always zero.

- **Detector**:
  Combination of the name of the module, the `input` and `output` parameter (both defaulting to an empty string) and the
  name of detector this module is executed for. If the name of the detector is specified directly by the `name` parameter,
  the priority is *high*. If the detector is only matched by the `type` parameter, the priority is *medium*. If the `name`
  and `type` are both unspecified and the module is instantiated for all detectors, the priority is *low*.

In the end, only a single instance for every unique name is allowed. If there are multiple instantiations with the same
unique name, the instantiation with the highest priority is kept. If multiple instantiations with the same unique name and
the same priority exist, an exception is raised.

## Multithreading: Parallel execution of events

The framework supports running several events in parallel via its multithreading feature. By default, this feature is
disabled for new modules. If supported by all modules in the simulation, multithreading is enabled by default, but can be
disabled by the user as described in [Section 3.4](../03_getting_started/04_framework_parameters.md). When enabled this
feature can provide a significant speed improvement, depending on the simulation chain.

The framework allows to parallelize the execution of the same module for multiple events, if these would otherwise be
executed directly after each other in a linear order. Thus, events are added to a work queue and then distributed to a set of
worker threads as specified in the configuration or determined from system parameters.

Detailed description of how the framework implements the multithreading feature can be found in
[Section 4.10](./10_multithreading.md) and an overview of important considerations when writing a new module capable of
multithreading is provided in [Section 10.4](../10_development/04_thread_safe_code.md).


[@linuxld]: http://man7.org/linux/man-pages/man8/ld.so.8.html
