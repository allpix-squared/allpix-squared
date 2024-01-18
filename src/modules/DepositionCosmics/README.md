---
# SPDX-FileCopyrightText: 2021-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "DepositionCosmics"
description: "Energy deposition from cosmic rays"
module_status: "Functional"
module_maintainers: ["Simon Spannagel (<simon.spannagel@cern.ch>)"]
module_outputs: ["DepositedCharge", "MCParticle", "MCTrack"]
---

## Description

This module simulates cosmic ray particle shower distributions and their energy deposition in all sensors of the setup.
The cosmic ray particle showers are simulated using the Cosmic-ray shower generator (CRY) \[[@cry]\], the generated particles are transported through the setup by Geant4.
More detailed information about CRY can be found in its physics description \[[@cryphysics]\] and user manual \[[@crymanual]\].

This module inherits functionality from the *DepositionGeant4* module and several of its parameters have their origin there.
A detailed description of these configuration parameters can be found in the respective module documentation.
The parameter `number_of_particles` here refers to full shower developments instead of individual particles, there can be multiple particles per shower.
The number of electron/hole pairs created by a given energy deposition is calculated using the mean pair creation energy \[[@chargecreation]\], fluctuations are modeled using a Fano factor assuming Gaussian statistics \[[@fano]\].
Default values of both parameters for different sensor materials are included and automatically selected for each of the detectors. A full list of supported materials can be found elsewhere in the manual.
These can be overwritten by specifying the parameters `charge_creation_energy` and `fano_factor` in the configuration.

The coordinate system for this module defines the `z` axis orthogonal to the earth surface, pointing upwards.
This means shower particles travel along the negative `z` axis and all detectors should be placed below the incidence plane at `z = 0`.
The area on which incident particles will be simulated is automatically inferred from the total setup size, and the next larger set of tabulated data available is selected.
Data are tabulated for areas of 1m, 3m, 10m, 30m, 100m, and 300m. Particles outside the selected window are dropped.

The first shower particle arriving in the event has a timestamp of `0ns`, all subsequent particles of the same shower have the appropriate spacing in time.
It should be noted that the time difference between the arrival of different particles of the same shower can amount up to hundreds of microseconds.
If this behavior is not desired, all particle timestamps can be forced to `0ns` by enabling the `reset_particle_time` switch.

The total time elapsed in the CRY simulation for the given number of showers is stored in the module configuration under the key `total_time_simulated`. If the ROOTObjectWriter is used to store the simulation result, this value is available from the output file.
In other cases, the value can be obtained from the log output of the run.

## Dependencies

This module inherits from and therefore requires the *DepositionGeant4* module as well as an installation Geant4.

## Parameters

* `data_path`: Directory to read the tabulated input data for the CRY framework from. By default, this is the standard installation path of the data files shipped with the framework.
* `reset_particle_time`: Boolean to force resetting all particle timestamps to `0ns`, even from different particles from the same shower. Defaults to `false`, i.e. the first particle of a shower bears a timestamp of `0ns` and all subsequent particles retain their time difference to the first one.

### Relevant parameters inherited from *DepositionGeant4*

* `physics_list`: Geant4-internal list of physical processes to simulate, defaults to FTFP_BERT_LIV. More information about possible physics list and recommendations for defaults are available on the Geant4 website \[[@g4physicslists]\].
* `enable_pai`: Determines if the Photoabsorption Ionization model is enabled in the sensors of all detectors. Defaults to false.
* `pai_model`: Model can be **pai** for the normal Photoabsorption Ionization model or **paiphoton** for the photon model. Default is **pai**. Only used if *enable_pai* is set to true.
* `charge_creation_energy` : Energy needed to create a charge deposit. Defaults to the energy needed to create an electron-hole pair in the respective sensor material (e.g. 3.64 eV for silicon sensors, \[[@chargecreation]\]). A full list of supported materials can be found elsewhere in the manual.
* `fano_factor`: Fano factor to calculate fluctuations in the number of electron/hole pairs produced by a given energy deposition. Defaults are provided for different sensor materials, e.g. a value of 0.115 for silicon \[[@fano]\]. A full list of supported materials can be found elsewhere in the manual.
* `max_step_length` : Maximum length of a simulation step in every sensitive device. Defaults to 1um.
* `range_cut` : Geant4 range cut-off threshold for the production of gammas, electrons and positrons to avoid infrared divergence. Defaults to a fifth of the shortest pixel feature, i.e. either pitch or thickness.
* `cutoff_time` : Maximum lifetime of particles to be propagated in the simulation. This setting is passed to Geant4 as user limit and assigned to all sensitive volumes. Particles and decay products are only propagated and decayed up the this time limit and all remaining kinetic energy is deposited in the sensor it reached the time limit in. Defaults to 221s (to ensure proper gamma creation for the Cs137 decay).
Note: Neutrons have a lifetime of 882 seconds and will not be propagated in the simulation with the default `cutoff_time`.
* `number_of_particles` : Number of cosmic ray showers to generate in a single event. Defaults to one.
* `output_plots` : Enables output histograms to be generated from the data in every step (slows down simulation considerably). Disabled by default.
* `output_plots_scale` : Set the x-axis scale of the output plot, defaults to 100ke.

### CRY Framework Parameters

* `latitude`: Latitude for which the incident particles from cosmic ray showers should be simulated. Should be between `90.0` (north pole) and `-90.0` (south pole). Defaults to `53.0` (DESY).
* `date`: Date for the simulation to account for the 11-year cycle of solar activity and related change in cosmic ray flux. Should be given as string in the form `month-day-year` and defaults to the last day of 2020, i.e. `12-31-2020`.
* `return_neutrons`: Boolean to select whether neutrons should be returned to Geant4. Defaults to `true`.
* `return_protons`: Boolean to select whether protons should be returned to Geant4. Defaults to `true`.
* `return_gammas`: Boolean to select whether gammas should be returned to Geant4. Defaults to `true`.
* `return_electrons`: Boolean to select whether electrons should be returned to Geant4. Defaults to `true`.
* `return_muons`: Boolean to select whether muons should be returned to Geant4. Defaults to `true`.
* `return_pions`: Boolean to select whether pions should be returned to Geant4. Defaults to `true`.
* `return_kaons`: Boolean to select whether kaons should be returned to Geant4. Defaults to `true`.
* `altitude`: Altitude for which the shower particles should be simulated. Possible values are `0m`, `2100m` and `11300m`, defaults to sea level, i.e. `0m`. It should be noted that the particle incidence plane is always located at `z = 0` independent of the simulated altitude.
* `min_particles`: Minimum number of particles required for a shower to be considered. Defaults to `1`.
* `max_particles`: Maximum number of particles in a shower before additional particles are cut off. Defaults to `100000`
* `area`: Side length of the squared area for which incident particles are simulated. This can maximally be `300m`. By default, the maximum size is automatically derived from the dimensions of the detector setup of the current simulation.

## Usage

```ini
[DepositionCosmics]
physics_list = FTFP_BERT_LIV
number_of_particles = 2
max_step_length = 10.0um
return_kaons = false
altitude = 0m
```

## Licenses

CRY is published under a 3-Clause BSD-like license, which is available in the file `cry/COPYRIGHT.TXT`.
The original software can be obtained from https://nuclear.llnl.gov/simulation/.

[@cry]: https://ieeexplore.ieee.org/abstract/document/4437209
[@cryphysics]: https://nuclear.llnl.gov/simulation/doc_cry_v1.7/cry_physics.pdf
[@crymanual]: https://nuclear.llnl.gov/simulation/doc_cry_v1.7/cry.pdf
[@g4physicslists]: https://geant4-userdoc.web.cern.ch/UsersGuides/PhysicsListGuide/html/index.html
[@chargecreation]: https://doi.org/10.1103/PhysRevB.1.2945
[@fano]: https://doi.org/10.1103%2FPhysRevB.22.5565
