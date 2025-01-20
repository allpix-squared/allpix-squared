---
# SPDX-FileCopyrightText: 2018-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "MagneticFieldReader"
description: "Magnetic field loading for a detector"
module_status: "Functional"
module_maintainers: ["Paul Schuetze (<paul.schuetze@desy.de>)"]
---

## Description

Unique module, adds a magnetic field to the full volume, including the active sensors. By default, the magnetic field is turned off.

The magnetic field reader only provides constant magnetic fields, read in as a three-dimensional vector. The magnetic field is forwarded to the GeometryManager, enabling the magnetic field for the particle propagation via Geant4, as well as to all detectors for enabling a Lorentz drift during the charge propagation.

## Parameters

* `model` : Type of the magnetic field model, currently only **constant** possible.
* `magnetic_field` : Vector describing the magnetic field.

## Usage

An example is given below

```ini
[MagneticFieldReader]
model = "constant"
magnetic_field = 500mT 3.8T 0T
```
