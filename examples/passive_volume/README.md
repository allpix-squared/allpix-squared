---
# SPDX-FileCopyrightText: 2020-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Passive Volume"
description: "Simulation example with passive volumes in the geometry file"
---

This example showcases the definition of passive volumes in the geometry file.

The file `example_detector_passive.conf` contains a detector of the type `test`, as well as several passive objects, identified via the key-parameter pair `role = "passive"`.
The example shows the three basic objects implemented, while for the volume "sphere1" the "box1" is defined as `mother_volume`.
This implies that the sphere is integrated into the box and that the given position (and orientation, if applicable) are interpreted as specifications relative to the position and orientation of the box mother volume.

Optionally, the `VisualizationGeant4` can be used to visualize these objects.

All other modules operate with standard parameters.
