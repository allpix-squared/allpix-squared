---
# SPDX-FileCopyrightText: 2018-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Magnetic Field"
description: "Simulation with a magnetic field applied"
---

This example demonstrates the charged particle propagation inside a sensor with a magnetic field applied.

Two CMS Pixel Detector single chip modules are placed in a 3.8 T magnetic field, of which the rear one is turned to 19 deg. This results in mostly 2 pixel clusters in the front sensor due to the Lorentz drift, while the rotation of the second sensor cancels out the Lorentz drift, resulting in mostly 1 pixel clusters.

For better performance, disable the output plots for the `GenericPropagation` module.
