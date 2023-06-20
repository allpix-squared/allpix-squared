---
# SPDX-FileCopyrightText: 2022-2023 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "DepositionGenerator"
description: "Energy deposition with data read from MC Generator Data Files"
module_status: "Functional"
module_maintainers: ["Simon Spannagel (<simon.spannagel@cern.ch>)"]
module_outputs: ["DepositedCharge", "MCParticle", "MCTrack"]
---

## Description

This module allows to read primary particles produced by Monte Carlo event generators from files in different data formats, and to emit them to a Geant4 `ParticleGun`.
The particles are then tracked through the setup using Geant4, and the resulting energy deposits are converted to `DepositedCharge` objects and dispatched to the subsequent simulation chain.
The different file formats can be selected via the `model` parameter, the path to the data file has to be provided via the `file_name` configuration parameter.

Events are read consecutively from the generator event data and event number are matched. This means that the event with number 5 in Allpix Squared will contain the data from event number 5 of the generator data file. If events are missing in the generator data, no primary particles are generated in Allpix Squared and the event remains empty.

This module inherits functionality from the *DepositionGeant4* module and several of its parameters have their origin there.
A detailed description of these configuration parameters can be found in the respective module documentation.
The number of electron/hole pairs created by a given energy deposition is calculated using the mean pair creation energy [@chargecreation], fluctuations are modeled using a Fano factor assuming Gaussian statistics [@fano].
Default values of both parameters for different sensor materials are included and automatically selected for each of the detectors. A full list of supported materials can be found elsewhere in the manual.
These can be overwritten by specifying the parameters `charge_creation_energy` and `fano_factor` in the configuration.

## Dependencies

This module inherits from and therefore requires the *DepositionGeant4* module as well as an installation of Geant4.
In addition, an installation of the HepMC3 library is required for the module to support the formats `HepMC3`, `HepMC2`, `HepMC ROOTIO` as well as `HepMC ROOTIO TTree`.

## Parameters

* `model`: Input data model. Currently supported is the data format of the [@genie] Monte Carl generator (`GENIE`) as well as the `HepMC3`, `HepMC2`, `HepMCROOT`, `HepMCTTree` data formats written by the HepMC3 library [@hepmc3].
* `file_name`: Path to the input data file to be read.

### Relevant parameters inherited from *DepositionGeant4*

* `physics_list`: Geant4-internal list of physical processes to simulate, defaults to FTFP_BERT_LIV. More information about possible physics list and recommendations for defaults are available on the Geant4 website [@g4physicslists].
* `enable_pai`: Determines if the Photoabsorption Ionization model is enabled in the sensors of all detectors. Defaults to false.
* `pai_model`: Model can be **pai** for the normal Photoabsorption Ionization model or **paiphoton** for the photon model. Default is **pai**. Only used if *enable_pai* is set to true.
* `charge_creation_energy` : Energy needed to create a charge deposit. Defaults to the energy needed to create an electron-hole pair in the respective sensor material (e.g. 3.64 eV for silicon sensors, [@chargecreation]). A full list of supported materials can be found elsewhere in the manual.
* `fano_factor`: Fano factor to calculate fluctuations in the number of electron/hole pairs produced by a given energy deposition. Defaults are provided for different sensor materials, e.g. a value of 0.115 for silicon [@fano]. A full list of supported materials can be found elsewhere in the manual.
* `max_step_length` : Maximum length of a simulation step in every sensitive device. Defaults to 1um.
* `range_cut` : Geant4 range cut-off threshold for the production of gammas, electrons and positrons to avoid infrared divergence. Defaults to a fifth of the shortest pixel feature, i.e. either pitch or thickness.
* `cutoff_time` : Maximum lifetime of particles to be propagated in the simulation. This setting is passed to Geant4 as user limit and assigned to all sensitive volumes. Particles and decay products are only propagated and decayed up the this time limit and all remaining kinetic energy is deposited in the sensor it reached the time limit in. Defaults to 221s (to ensure proper gamma creation for the Cs137 decay).
Note: Neutrons have a lifetime of 882 seconds and will not be propagated in the simulation with the default `cutoff_time`.
* `output_plots` : Enables output histograms to be generated from the data in every step (slows down simulation considerably). Disabled by default.
* `output_plots_scale` : Set the x-axis scale of the output plot, defaults to 100ke.

## Usage

```ini
[DepositionGenerator]
physics_list = FTFP_BERT_LIV
max_step_length = 10.0um
model = "GENIE"
file_name = "genie_input_data.root"
```

[@genie]: https://doi.org/10.1016/j.nima.2009.12.009
[@hepmc3]: https://doi.org/10.1016/j.cpc.2020.107310
[@chargecreation]: https://doi.org/10.1103/PhysRevB.1.2945
[@fano]: https://doi.org/10.1103%2FPhysRevB.22.5565
[@g4physicslists]: https://geant4-userdoc.web.cern.ch/UsersGuides/PhysicsListGuide/html/index.html
