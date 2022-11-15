---
# SPDX-FileCopyrightText: 2022 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "DepositionLaser"
description: "A simplistic deposition generator for charge injection with a laser."
module_maintainer: "Daniil Rastorguev (<daniil.rastorguev@desy.de>)"
module_status: "Functional"
module_output: "DepositedCharge, MCParticle"
---

## Description

This deposition generator is mostly intended for simulations of laser-TCT experiments.
It would generate charge, deposited by absorption of a laser pulse in silicon bulk.

Current implementation assumes that the laser pulse is a bunch of point-like photons, each traveling in a straight line. A lookup table \[[@optical_properties]\] is used to determine absorption and refraction coefficients for a given wavelength.

This module has its own tracking algorithms (no dependencies on Geant4), based on Allpix geometry
Tracking features:
* photons are absorbed in detector bulk on physically correct depth
* photons refract on silicon-air interface
* tracks are terminated when a photon leaves *first encountered* sensitive volume
* tracks are terminated if a passive object is hit (the only supported passive object type is `box`)

Spatial and temporal distribution of incident photons are tuned to mimic a real laser pulse.
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

* `output_plots`, defaults to `false`

## Usage
`DepositedCharge` instances created by this module are properly assigned time.
Thus, a simulation pipeline with `DepositionLaser`, `TransientPropagation` and `PulseTransfer` is expected to produce pulse shapes, comparable with experimentally obtained ones.

[@optical_properties]: https://doi.org/10.1002/pip.4670030303
