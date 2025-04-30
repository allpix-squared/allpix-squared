---
# SPDX-FileCopyrightText: 2018-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "MagneticFieldReader"
description: "Magnetic field loading for a detector"
module_status: "Functional"
module_maintainers: ["Paul Schuetze (<paul.schuetze@desy.de>)"]
---

## Description

Unique module, adds a magnetic field to the full volume of the simulation, including the active sensors.
By default, the magnetic field is turned off.

The magnetic field reader provides the possibility of using a simple constant magnetic field permeating the entire simulated setup,
or a meshed field which is centered around the origin of the global coordinate system. The magnetic field is forwarded to the
GeometryManager, enabling the magnetic field for the particle propagation via Geant4, as well as to all detectors for
enabling a Lorentz drift during the charge propagation.

For the **constant** model, the field is set as a three-dimensional vector.

For the **mesh** model, the field needs to be provided in form of an APF or INIT file which provides total size of the field,
the bin size of the mesh as well as the actual field data. In addition, an offset of this field from the origin of the global
coordinate system can be provided via the `offset` parameter. Here, the value set via `magnetic_field` is sued as a fallback
field value used outside the volume provided by the field file.

## Parameters

* `model` : Type of the magnetic field model, possible values are **constant** and **mesh**.
* `magnetic_field` : Vector describing the magnetic field. In the model **mesh** this is used as the fallback field outside the meshed region.
* `file_name` : Path to the APF or INIT file containing the magnetic field to be used. Only used in the **mesh** model.
* `offset` : Offset of the meshed magnetic field center from the global center of origin of the simulation. Defaults to `0, 0, 0` and is only used in the **mesh** model.

## Usage

An example for a constant magnetic field is

```ini
[MagneticFieldReader]
model = "constant"
magnetic_field = 500mT 3.8T 0T
```

The configuration for a meshed field may look like the following:

```ini
[MagneticFieldReader]
model = "mesh"
magnetic_field = 500mT 0T 0T
file_name = "path/to/magnetic_field.apf"
offset = 5cm, 6cm, 4mm
```
