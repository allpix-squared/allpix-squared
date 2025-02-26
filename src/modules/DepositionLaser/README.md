---
# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "DepositionLaser"
description: "A simplistic deposition generator for charge injection with a laser"
module_status: "Functional"
module_maintainers: ["Daniil Rastorguev (<daniil.rastorguev@desy.de>)"]
module_outputs: ["DepositedCharge", "MCParticle"]
---

## Description

This deposition generator is mostly intended for simulations of laser-TCT experiments.
It generates charge, deposited by absorption of a laser pulse in detector bulk.
This module is not dependent on Geant4. Instead, it implements tracking algorithms and simulations of corresponding physical phenomena by its own, using internal Allpix geometry.

Current implementation assumes that the laser pulse is a bunch of point-like photons, each traveling in a straight line. A lookup table \[[@optical_properties]\] is used to determine absorption and refraction coefficients in silicon for a given wavelength.
The only supported sensor material is silicon, unless optical properties are explicitly set (see *Parameters*).

Multiple photons are produced in *one* event, thus a *single* event models a *single* laser pulse.


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
For a `converging` beam, track directions would have isotropic distribution (but with a limit on a max angle between the
track and the set beam direction).

The transversal width of the Gaussian beam is defined by the beam waist $`w_0`$, which describes the minimal beam width. The beam width in turn is defined as the distance between the point of maximum intensity and the point where the intensity drops to $`\frac{1}{e^2}`$. For a Gaussian-distributed intensity, the intensity drops to $`\frac{1}{e^2}`$, if $`x=2\sigma`$, see the probability distribution function of the normal distribution. The beam waist therefore equals $`2\sigma`$.

**NB**: convention on global time zero for this module contradicts the general convention of the Allpix Squared.
For this module, global t=0 is chosen in such a way that the mean value of temporal distribution is *always* positioned at
*4 standard deviations*  w.r.t. the global t=0.
Thus, there is not necessarily a particle that is created exactly when the global time starts.
Although, the following Allpix Squared conventions still apply:

* No particles have a negative timestamp.
* Local time zero for each detector is a moment when the first particle that creates a hit in this detectors enters its bulk.

As a result, this module yields `DepositedCharge` instances for each detector, with them having physically correct spatial
and temporal distribution.


## Parameters

* `number_of_photons`: number of incident photons, generated in *one* event. Defaults to 10000. The total deposited charge
  will also depend on wavelength and geometry.
* `group_photons`: if specified, incident photons will be grouped in buckets of given size, decreasing amount of `DepositedCharge` instances (but keeping total amount of deposited charge the same), thus reducing load on the propagation module.
* `wavelength` of the laser. If specified, it is used to retrieve sensor optical properties from the lookup table (data is available for the range of 250 -- 1450 nm). The only supported material is silicon.
* `data_path`: Directory to read the tabulated input data for the absorption on silicon. By default, this is the standard installation path of the data files shipped with the framework.
* `absorption_length` and `refractive_index`: if both are specified, given values are used instead of the lookup table. This also allows use of sensor materials other than silicon.
* `pulse_duration`: gaussian width of pulse temporal profile. Defaults to 0.5 ns.
* `source_position`: a 3D position vector.
* `beam_direction`: a 3D direction vector.
* `beam_geometry`: either `cylindrical` or `converging`
* `beam_waist`: parametrises the transversal width of the beam by the beam waist $`w_0=2\sigma`$ described above. Defaults to 20 um.
* `focal_distance`: needs to be specified for `converging` beam. This distance is *as it would be in air*. In silicon, beam
  shape will effectively stretch along its direction due to refraction and the actual focus will be further away from the
  source.
* `beam_convergence_angle`: max angle between tracks and `beam_direction`. Needs to be specified for a `converging` beam.
* `output_plots`: if set `true`, this module will produce histograms to monitor beam shape and also 3D distributions of charges, deposited in each detector. Histograms would look sensible even for one-event runs. Defaults to `false`.


## Usage

A simulation pipeline to build an analog detector response would include `DepositionLaser`, `TransientPropagation` and
`PulseTransfer`.
Usually it is enough to run just a single event (or a few).
While multithreading is supported by this module, one should note that the pipeline for each event is quite computationally
intensive and runs with only one event do not gain any additional performance from multi-threaded execution.

Such pipeline is expected to produce pulse shapes, comparable with experimentally obtained ones. An example of
`DepositionLaser` configuration is shown below.

```ini
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
