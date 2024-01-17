---
# SPDX-FileCopyrightText: 2020-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "EUDET Telescope"
description: "Simulation of a EUDET-type beam telescope"
---

This example demonstrates the simulation of EUDET-type Beam Telescopes, making use of the `ProjectionPropagation` module and approximated simulation and sensor parameters tuned to measurements.

The simulation setup represents a beam telescope consisting of six Mimosa26 sensors, MAPS sensors with a small depletion zone. The thickness used herein is 50 um.

The particle propagation and charge deposition are performed via the `DepositionGeant4` module, using an electron beam with an energy of 5 GeV. A standard physics list is chosen.

The electric field of the sensor is approximated with a depletion depth of 15 um and a bias voltage of -4 V.

The charge collection in such sensors is heavily influenced by the charge carriers that are not created in, but diffuse into the depleted volume, often with a strong lateral component leading to cluster sizes larger than one. To approximate this behaviour and obtain a realistic detector response, while simultaneously maintaining a low simulation time, the `diffuse_deposit` parameter of the `ProjectionPropagation` module is used. An integration time of 20 ns is used as an approximation, tuned to experimental data.

The digitization parameters correspond to information from the sensor developers.
