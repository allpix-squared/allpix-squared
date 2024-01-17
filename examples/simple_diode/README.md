---
# SPDX-FileCopyrightText: 2019-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Simple Diode"
description: "Am241 source measurement with a paper collimator over a diode"
---

This example simulates an Am241 alpha source using a native Geant4 GPS macro. The source is defined as a disk from which mono-energetic 5.4 MeV alphas are emitted. This approximates the Am241 alpha spectrum. The source emits the alpha particles isotropically.

A diode-type detector is placed below the source, shielded with an additional sheet of paper of 200um thickness with a pinhole in it to let the alpha pass. The goal is to reproduce the aperture effect seen with alpha particles, where the detected spectrum shows a dependency on the pinhole size due to the different path lengths of the alpha particles in the air as a function of the incident angle at the diode. Small pinhole diameters restrict the incidence angles to be more or less vertical, while larger pinhole diameters also allow alphas at larger angles to pass. The resulting longer path length of these particles results in a larger energy loss before reaching the diode detector.

The charge deposition is performed by Geant4 using a standard physics list and a stepping length of 10um.
The `ProjectionPropagation` module with a setting of `charge_per_step = 500` is used to simulate the charge carrier propagation and the simulation result is stored to file. The `model_paths` parameter is set to add this directory to the search path for detector models.

Optionally, the `VisualizationGeant4` can be used to visualize these objects.
