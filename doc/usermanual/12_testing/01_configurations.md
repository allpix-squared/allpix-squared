---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Test Configurations"
weight: 1
---

Test configuration files consist of regular Allpix Squared configuration files for a simulation, invoking the desired
behavior to be tested. In addition, test files can contain tags and pass conditions as described in the subsequent section.

CMake automatically searches for Allpix Squared configuration files in the different directories and passes them to the
Allpix Squared executable (cf. [Section 3.5](../03_getting_started/05_allpix_executable.md)). Adding a new test is as simple
as adding a new configuration file to the respective directories and specifying the pass or fail conditions based on the tags
described in the following paragraphs.

Three different types of tests are distinguished:

## Framework Functionality Tests

The framework functionality tests validate the core framework components such as seed distribution, multithreading
capabilities, configuration parsing and coordinate transformations. The configuration of the tests can be found in the
`etc/unittests/test_core` directory of the repository and are automatically discovered by CMake.

## Module Functionality Tests

These tests are meant to ensure proper functioning of an individual module given a defined input and thus protect the
framework against accidental changes affecting the physics simulation. Using a fixed seed (using the `random_seed`
configuration keyword) together with a specific version of Geant4 \[[@geant4]\], if necessary, allows to reproduce the same
simulation event.

Typically, one single event is produced per test and the `DEBUG`-level logging output of the respective module is checked
against pre-defined expectation output using regular expressions. Once modules are altered, their respective expectation
output has to be adapted after careful verification of the simulation result.

Module tests are located within the individual module source folders and are only enabled if the respective module will be
built. For new modules, the directory in which the test files are located needs to be registered in the main CMake file of
the module as described in [Section 10.3](../10_development/03_new_module.md#files-of-a-module). Module test files have to
start with a two-digit number and end with the file extension `.conf`, e.g. `01-mytest.conf`, to be detected.

## Performance Tests

These tests run a set of simulations on a dedicated machine to catch any unexpected prolongation of the simulation time, e.g.
by an accidentally introduced heavy operation in a hot loop. Performance tests use configurations prepared such, that one
particular module takes most of the load (dubbed the *slowest instantiation* by Allpix Squared), and a few of thousand events
are simulated starting from a fixed seed for the pseudo-random number generator. The `#TIMEOUT` keyword in the configuration
file will ask CTest to abort the test after the given running time.

In the project CI, performance tests are limited to native runners, i.e. they are not executed on docker hosts where the
hypervisor decides on the number of parallel jobs. Only one test is performed at a time.

Despite these countermeasures, fluctuations on the CI runners occur, arising from different loads of the executing machines.
Thus, all performance CI jobs are marked with the `allow_failure` keyword which allows GitLab to continue processing the
pipeline but will mark the final pipeline result as *passed with warnings* indicating an issue in the pipeline. These tests
should be checked manually before merging the code under review.

The configuration of the tests can be found in the `etc/unittests/test_performance` directory of the repository and are
automatically discovered by CMake.


[@geant4]: https://doi.org/10.1016/S0168-9002(03)01368-8
