---
# SPDX-FileCopyrightText: 2022 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "DepositionLaser"
description: "A simplistic deposition generator for charge injection with a laser."
module_maintainer: "Daniil Rastorguev (<daniil.rastorguev@desy.de>)"
module_status: "Immature"
# module_output: "DepositedCharge, MCParticle"
---

## Description

This deposition generator is mostly intended for simulations of laser-TCT experiments.
This module implements physics algorithms by itself, without relying on external libraries (e.g., Geant4).

Current implementation assumes that laser pulse is a bunch of point-like photons, each traveling in a straight line. As they cross sensitive silicon volumes, they could be absorbed, respecting exponential distribution of penetration depth. A lookup table ([[1]](#1)) is used to determine absorption coefficients for a given wavelength.

Laser simulation features:
* all passive volumes are currently ignored;
* although, behavior of stacked detectors would be correct;
* gaussian beam shape could be simulated; if so, angular and spatial distribution of straight-line-photons is set to mimic intensity distribution in a gaussian beam

Advanced tracking will be implemented later. This will include:
* termination of tracks if a passive object is hit;
* photon refraction as they cross silicon surface.

As a result, this module yields multiple `DepositedCharge` instances for each detector, with them having physically correct spatial and temporal distribution.




## Parameters
* `number_of_photons`: number of photons in a *single* event, defaults to 10000
* `source_position`, a 3D position vector
* `beam_direction`, a 3D direction vector
* `wavelength` of a laser supported values are 250 -- 1450 nm
* `pulse_duration`: gaussian width of pulse temporal profile

* `beam_waist`: std_dev of transversal beam profile, defaults to 20 um
* `beam_geometry`, either `cylindrical` or `converging`
* `focal_distance`(length) and `beam_convergence_angle`: both need to be specified for a `converging` beam

* `verbose_tracking`, defaults to `false`; if set `true`, it will increase amount of tracking-related debug log output (and may slightly increase computing cost if multiple detectors are present)
* `output_plots`, defaults to `false`

## Usage
*Example how to use this module*

## References
<a id="1">[1]</a>
M. A. Green and Keevers, M. J., “Optical properties of intrinsic silicon at 300 K”,
Progress in Photovoltaics: Research and Applications, vol. 3, pp. 189 - 192, 1995.
