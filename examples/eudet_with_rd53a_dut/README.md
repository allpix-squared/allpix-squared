---
# SPDX-FileCopyrightText: 2021-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "EUDET with RD53a DUT"
description: "Simulation of the DESY testbeam setup with a EUDET telescope and RD53a modules"
---

This example is similar to the EUDET-type telescope example but with extra DUTs added to match the DESY testbeam setup with RD53a modules.
The setup consists of six telescope planes of MIMOSA26-type (EUDET beam telescope) and two RD53a modules centered in between the telescope arms:
DUT0 defined with 50x50um^2 pitch and DUT1 defined with 20x100um^2 pitch.
Furthermore, one FEI4 reference plane is added as the last plane as in real testbeam setup.

The goal of this setup is to simulate the performance of RD53a modules with testbeam setup and to study multiple scattering effects with passive and extra material.
For this purpose, a box made of plexiglass is introduced in the geometry, but the user can also try other materials within the same range of radiation length, such as polystyrene or styrofoam.

A linear electric field is applied to all sensors, with the DUTs and the reference plane on a higher bias voltage than the telescope planes.
More complex electric fields can be added by the user by altering the configuration of the respective `ElectricFieldReader` modules.
The propagation of charge carriers is performed using the `ProjectionPropagation` module for the MIMOSA26 sensors of the EUDET telescope planes, and using the `GenericPropagation` module for the DUT and reference planes.
This ensures minimum computing requirements for the telescope planes while providing a more detailed simulation for the detectors of interest.

The `LCIOWriter` module is placed at the end of the simulation chain in order to write the results of the simulation to a file in  the LCIO format that can be used as input for the reconstruction software EUTelescope.
Here, it is important to assign the name `zsdata` to the respective data for EUTelescope to properly recognize it.
In order to reconstruct the simulation with the Corryvreckan framework, the user can replace this module with the `CorryvreckanWriter` module.
