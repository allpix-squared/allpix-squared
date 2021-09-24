# DepositionCosmics
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Immature  
**Output**: DepositedCharge, MCParticle, MCTrack

### Description

This module simulates cosmic ray particle shower distributions and their energy deposition in all sensors of the setup.
The deposited energy is converted into electron-hole pairs, the number of pairs created is calculated using the mean pair creation energy `charge_creation_energy`, fluctuations are modeled using a Fano factor `fano_factor` assuming Gaussian statistics.

This module inherits functionality from the DepositionGeant4 modules and several of its parameters have their origin there. A detailed description of these configuration parameters can be found in the respective module documentation.
The parameter `number_of_particles` here refers to full shower developments instead of individual particles, there can be multiple particles per shower.

The coordinate system for this module defines the `z` axis orthogonal to the earth surface, pointing towards the earth core.
The area on which incident particles will be simulated is automatically inferred from the total setup size, and the next larger set of tabulated data available is selected.
Data are tabulated for areas of 1m, 3m, 10m, 30m, 100m, and 300m. Particles outside the selected window are dropped.

### Dependencies

This module inherits from and therefore requires the *DepositionGeant4* module as well as an installation Geant4.

### Parameters

* `data_path`: Directory to read the tabulated input data for the CRY framework from. By default, this is the standard installation path of the data files shipped with the framework.

#### Revelant parameters inherited from *DepositionGeant4*

* `physics_list`: Geant4-internal list of physical processes to simulate, defaults to FTFP_BERT_LIV. More information about possible physics list and recommendations for defaults are available on the Geant4 website [@g4physicslists].
* `enable_pai`: Determines if the Photoabsorption Ionization model is enabled in the sensors of all detectors. Defaults to false.
* `pai_model`: Model can be **pai** for the normal Photoabsorption Ionization model or **paiphoton** for the photon model. Default is **pai**. Only used if *enable_pai* is set to true.
* `charge_creation_energy` : Energy needed to create a charge deposit. Defaults to the energy needed to create an electron-hole pair in silicon (3.64 eV, [@chargecreation]).
* `fano_factor`: Fano factor to calculate fluctuations in the number of electron/hole pairs produced by a given energy deposition. Defaults to 0.115 [@fano].
* `max_step_length` : Maximum length of a simulation step in every sensitive device. Defaults to 1um.
* `range_cut` : Geant4 range cut-off threshold for the production of gammas, electrons and positrons to avoid infrared divergence. Defaults to a fifth of the shortest pixel feature, i.e. either pitch or thickness.
* `cutoff_time` : Maximum lifetime of particles to be propagated in the simulation. This setting is passed to Geant4 as user limit and assigned to all sensitive volumes. Particles and decay products are only propagated and decayed up the this time limit and all remaining kinetic energy is deposited in the sensor it reached the time limit in. Defaults to 221s (to ensure proper gamma creation for the Cs137 decay).
Note: Neutrons have a lifetime of 882 seconds and will not be propagated in the simulation with the default `cutoff_time`.
* `number_of_particles` : Number of cosmic ray showers to generate in a single event. Defaults to one.
* `output_plots` : Enables output histograms to be be generated from the data in every step (slows down simulation considerably). Disabled by default.
* `output_plots_scale` : Set the x-axis scale of the output plot, defaults to 100ke.

#### CRY Framework Parameters
* `latitude`: Latitude for which the incident particles from cosmic ray showers should be simulated. Should be between `90` (north pole) and `-90` (south pole). Defaults to `53` (DESY).
* `date`: Date for the simulation to account for the 11-year cycle of solar activity and related change in cosmic ray flux. Should be given as string in the form `month-day-year` and defaults to the last day of 2020, i.e. `12-31-2020`.
* `return_neutrons`: Boolean to select whether neutrons should be returned to Geant4. Defaults to `true`.
* `return_protons`: Boolean to select whether protons should be returned to Geant4. Defaults to `true`.
* `return_gammas`: Boolean to select whether gammas should be returned to Geant4. Defaults to `true`.
* `return_electrons`: Boolean to select whether electrons should be returned to Geant4. Defaults to `true`.
* `return_muons`: Boolean to select whether muons should be returned to Geant4. Defaults to `true`.
* `return_pions`: Boolean to select whether pions should be returned to Geant4. Defaults to `true`.
* `return_kaons`: Boolean to select whether kaons should be returned to Geant4. Defaults to `true`.
* `altitude`: Altitude for which the shower particles should be simulated. Possible values are `0m`, `2100m` and `11300m`, defaults to ground level, i.e. `0m`.
* `min_particles`: Minimum number of particles required for a shower to be considered. Defaults to `1`.
* `max_particles`: Maximum number of particles in a shower before additional particles are cut off. Defaults to `100000`

### Usage

```toml
[DepositionCosmics]
physics_list = FTFP_BERT_LIV
number_of_particles = 2
max_step_length = 10.0um
return_kaons = false
altitude = 0m
```
