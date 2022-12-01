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
This module is not dependent on Geant4. Instead, it implements tracking algorithms and simulations of corresponding physical phenomena by its own, using internal Allpix geometry.

Current implementation assumes that the laser pulse is a bunch of point-like photons, each traveling in a straight line. A lookup table \[[@optical_properties]\] is used to determine absorption and refraction coefficients for a given wavelength.

Multiple photons are produced in a *one* event, thus a *single* event models a *single* laser pulse.  


Tracking features:

* photons are absorbed in detector bulk on physically correct depth
* each photon is assumed to create exactly one e-h pair
* photons refract on silicon-air interface
* tracks are terminated when a photon leaves *first encountered* sensitive volume
* tracks are terminated if a passive object is hit (the only supported passive object type is `box`)
Verbose information on tracking for each photon is printed if this module is run with `DEBUG` logging level.

Initial direction and starting timestamp for every photon in the bunch are generated to mimic
spatial and temporal distributions of delivered intensity of a real laser pulse.

Two options for beam geometry are currently available: `cylindrical` and `converging`.
For both options, transversal beam profiles will have a gaussian shape.
For a `cylindrical` beam, all tracks are parallel to the set beam direction.
For a `converging` beam, track directions would have isotropic distribution (but with a limit on a max angle between the track and the set beam direction).

**NB**: convention on global time zero for this module contradicts the general convention of the Allpix Squared.
For this module, global t=0 is chosen in such a way that the mean value of temporal distribution is *always* positioned at *4 standard deviations*  w.r.t. the global t=0.
Thus, there is not necessarily a particle that is created exactly when the global time starts.
Although, the following Allpix Squared conventions still apply:

* No particles have a negative timestamp.
* Local time zero for each detector is a moment when the first particle that creates a hit in this detectors enters its bulk.

As a result, this module yields `DepositedCharge` instances for each detector, with them having physically correct spatial and temporal distribution.



## Parameters

* `number_of_photons`: number of incident photons, generated in *one* event. Defaults to 10000. The total deposited charge will also depend on wavelength and geometry.
* `wavelength` of the laser. Supported values are 250 -- 1450 nm.
* `pulse_duration`: gaussian width of pulse temporal profile. Defaults to 0.5 ns.
* `source_position`: a 3D position vector.
* `beam_direction`: a 3D direction vector.
* `beam_geometry`: either `cylindrical` or `converging`
* `beam_waist`: standard deviation of transversal beam intensity distribution at focus. Defaults to 20 um.
* `focal_distance`: needs to be specified for `converging` beam. This distance is *as it would be in air*. In silicon, beam shape will effectively stretch along its direction due to refraction and the actual focus will be further away from the source.
* `beam_convergence_angle`: max angle between tracks and `beam_direction`. Needs to be specified for a `converging` beam.
* `output_plots`: if set `true`, this module will produce histograms to monitor beam shape and also 3D distributions of charges, deposited in each detector. Histograms would look sensible even for one-event runs. Defaults to `false`.

## Usage
A simulation pipeline to build an analog detector response would include `DepositionLaser`, `TransientPropagation` and `PulseTransfer`.
Usually it is enough to run just a single event (or a few).
Multithreading is supported by this module.
One should note that for the pipeline above each event is very computation-heavy, and runs with just one event do not gain any additional performance from multi-threaded execution.

Such pipeline is expected to produce pulse shapes, comparable with experimentally obtained ones. An example of `DepositionLaser` configuration is shown below.

```
[Allpix]
detectors_file = "geometry.conf"
# A single event is often enough
number_of_events = 1
multithreading = false

[DepositionLaser]
log_level = "INFO"

# Standard wavelength for IR TCT lasers
wavelength = 1064nm

number_of_photons = 50000
pulse_duration = 1ns

# Geometry parameters of the beam
source_position = 0 0 -5mm
beam_direction = 0 0 1
beam_geometry = "converging"
beam_waist = 10um
focal_distance = 5mm
beam_convergence_angle = 20deg

output_plots = true
```


[@optical_properties]: https://doi.org/10.1002/pip.4670030303
