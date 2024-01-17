---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "ATLAS ITk Petal"
description: "Double-sided petal arrangement of radial strip detectors for the ATLAS ITk upgrade"
---

This example simulates a double-sided arrangement of radial strip detectors called a "petal". 
It features a total of 18 detectors (9 on each side) of various types designated as R0 to R5.
Such a structure is planned to be deployed in the end-cap regions of the ATLAS Inner Tracker (ITk). 
The entire petal structure can be viewed by enabling the `VisualizationGeant4` module.

In this simulation example, the detector is hit by a beam of 5.4 GeV electrons. 
A simple linear electric field model is used. Deposited charge carriers are grouped during the propagation stage by use of the `charge_per_step` parameter.
Charge threshold during the digitization phase is set to approximately three times the value of electronic noise.

The `granularity` parameter controls the number of bins for in-pixel histograms.
As the typical strip length of several centimeters would, by default, lead to exceedingly large histograms, the `granularity` parameter is set to `80 4`. 
This corresponds to a typical strip pitch of 80 um in the *x* direction and to 4 strip rows (in the *y* direction) of the radial strip detector.
