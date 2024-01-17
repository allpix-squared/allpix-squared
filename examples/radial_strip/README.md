---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Radial Strip Detector"
description: "Simulation of a silicon detector with radial strip electrodes"
---

This example simulates a radial strip detector, which features a trapezoidal shape and strips fanning out radially from a common focal point.
Such detectors are planned to be deployed in the end-cap regions of the ATLAS Inner Tracker (ITk).
This example uses the model of a specific ITk module, called the R0.
You can see the radial detector used in this simulation by enabling the `VisualizationGeant4` module.

Detector models for radial strip detectors are implemented in the `RadialStripDetectorModel` class. A radial strip detector model is defined using four parameters for each  strip row. In addition, its `geometry` has to be defined as `radial_strip`.

```ini
type = "monolithic"
geometry = "radial_strip"
number_of_strips = 1026, 1026, 1154, 1154
angular_pitch = 0.19309mrad, 0.19309mrad, 0.17169mrad, 0.17169mrad
inner_pitch = 74.4um, 78.1um, 73.6um, 78.5um
strip_length = 19mm, 24mm, 29mm, 32mm
```

In this simulation example, the detector is hit by a beam of 5.4 GeV electrons.
A simple linear electric field model is used. Deposited charge carriers are grouped during the propagation stage by use of the `charge_per_step` parameter.
The `CapacitiveTransfer` module handles the simulation of cross-talk by transferring a portion of collected charge to adjacent strips in the same row.
Charge threshold during the digitization phase is set to approximately three times the value of electronic noise.

The `granularity` parameter controls the number of bins for in-pixel histograms.
As the typical strip length of several centimeters would, by default, lead to exceedingly large histograms, the `granularity` parameter is set to `80 4`.
This corresponds to a typical strip pitch of 80 um in the *x* direction and to 4 strip rows (in the *y* direction) of the radial strip detector.
