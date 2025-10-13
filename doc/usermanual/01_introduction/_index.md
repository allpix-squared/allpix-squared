---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Introduction"
description: "A brief introduction to the Allpix Squared framework and its goals."
weight: 1
---

Allpix Squared is a generic simulation framework for semiconductor tracker and vertex detectors written in modern C++,
following the C++20 standard. The goal of the framework is to provide an easy-to-use package for simulating the performance
of semiconductor detectors, starting with the passage of ionizing radiation through the sensor and finishing with the
digitization of hits in the readout chip.

The framework builds upon other packages to perform tasks in the simulation chain, most notably Geant4 \[[@geant4]\] for the
deposition of charge carriers in the sensor and ROOT \[[@root]\] for producing histograms and storing the produced data. The
core of the framework focuses on the simulation of charge transport in semiconductor detectors and the digitization to hits
in the frontend electronics.

Allpix Squared is designed as a modular framework, allowing for an easy extension to more complex and specialized detector
simulations. The modular setup also allows to separate the core of the framework from the implementation of the algorithms in
the modules, leading to a framework which is both easier to understand and to maintain. Besides modularity, the framework was
designed with the following main design goals in mind:

1. Reflect the physics:

   - A run consists of several sequential events. A single event here refers to an independent passage of one or multiple
     particles through the setup.

   - Detectors are treated as separate objects for particles to pass through.

   - All relevant information must be contained at the end of processing every single event (sequential events).

2. Ease of use (user-friendly):

   - Simple, intuitive configuration and execution ("does what you expect").

   - Clear and extensive logging and error reporting capabilities.

   - Implementing a new module should be feasible without knowing all details of the framework.

3. Flexibility:

   - Event loop runs sequence of modules, allowing for both simple and complex user configurations.

   - Possibility to run multiple different modules on different detectors.

   - Limit flexibility for the sake of simplicity and ease of use.

Allpix Squared has been designed following some ideas previously implemented in the AllPix \[[@ap1wiki], [@ap1git]\] project.
Originally written as a Geant4 user application, AllPix has been successfully used for simulating a variety of different
detector setups.


[@geant4]: https://doi.org/10.1016/S0168-9002(03)01368-8
[@root]: http://root.cern.ch/
[@ap1wiki]: https://twiki.cern.ch/twiki/bin/view/Main/AllPix
[@ap1git]: https://github.com/ALLPix/allpix
