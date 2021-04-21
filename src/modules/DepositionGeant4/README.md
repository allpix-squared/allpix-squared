# DepositionGeant4
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>), Tobias Bisanz (<tobias.bisanz@phys.uni-goettingen.de>), Thomas Billoud (<thomas.billoud@cern.ch>)  
**Status**: Functional  
**Output**: DepositedCharge, MCParticle, MCTrack

### Description
Module which deposits charge carriers in the active volume of all detectors.
It acts as wrapper around the Geant4 logic and depends on the global geometry constructed by the GeometryBuilderGeant4 module.
It initializes the physical processes to simulate a particle source that will deposit charge carriers for every event simulated.
The number of electron/hole pairs created is calculated using the mean pair creation energy `charge_creation_energy`, fluctuations are modeled using a Fano factor `fano_factor` assuming Gaussian statistics.

#### Source Shapes

The source can be defined in two different ways using the `source_type` parameter: with pre-defined shapes or with a Geant4 macro file.
Pre-defined shapes are point, beam, square or sphere.
For the square and sphere, the particle starting points are distributed homogeneously over the surfaces.
By default, the particle directions for the square are random, as would be for a squared radioactive source.
For the sphere, unless a focus point is set, the particle directions follow the cosine-law defined by Geant4 [@g4gps] and the field inside the sphere is hence isotropic.

To define more complex sources or angular distributions, the user can create a macro file with Geant4 commands.
These commands are those defined for the GPS source and are explained in the Geant4 website [@g4gps].
In order to avoid collisions with internal configurations, the command `/gps/number` should be replaced by the configuration parameter `number_of_particles` in this module in order to correctly execute the Geant4 event loop.

All source positions defined in the macro via the commands `/gps/position` and `/gps/pos/centre` are used to automatically extend the Geant4 world volume to always include the sources.

#### Particles, Ions and Radioactive Decays

The particle type can be set via a string (`particle_type`) or by the respective PDG code (`particle_code`). Refer to the Geant4 webpage [@g4particles] for information about the available types of particles and the PDG particle code definition [@pdg] for a list of the available particles and PDG codes.

Radioactive sources can be simulated simply by setting their isotope name in the `particle_type` parameter, and optionally by setting the source energy to zero for a decay in rest.
The `G4RadioactiveDecay` package [@g4radioactive] is used to simulate the decay of the radioactive nuclei.
Secondary ions from the decay are not further treated and the decay chain is interrupted, e.g. Am241 only undergoes its alpha decay without the decay of its daughter nucleus of Np237 being simulated. The full decay chain can be simulated if the `cutoff_time` is set to the appropriate value for this chain.
Radioactive isotopes are forced to decay immediately in order to allow sensible measurements of arrival and deposition times.
Currently, the following radioactive isotopes are supported: `Fe55`, `Am241`, `Sr90`, `Co60`, `Cs137`.
Note that for `Cs137` the `cutoff_time` has to be set to 221 seconds for the decay to work properly.

Ions can be used as particles by setting their atomic properties, i.e. the atomic number Z, the atomic mass A, their charge Q and the excitation energy E via the following syntax:

```ini
particle_type = "ion/Z/A/Q/E"
```

where `Z` and `A` are unsigned integers, `Q` is a signed integer and `E` a floating point value with units, e.g. `13eV`.

#### Energy Deposition and Charge Carrier creation

For all particles passing the sensitive device of the detectors, the energy loss is converted into deposited charge carriers in every step of the Geant4 simulation.
Optionally, the Photoabsorption Ionization model (PAI) can be activated to improve the modeling of very thin sensors [@pai].
The information about the truth particle passage is also fully available, with every deposit linked to a MCParticle.
Each trajectory which passes through at least one detector is also registered and stored as a global MCTrack.
MCParticles are linked to their respective tracks and each track is linked to its parent track, if available.

A range cut-off threshold for the production of gammas, electrons and positrons is necessary to avoid infrared divergence.
By default, Geant4 sets this value to 700um or even 1mm, which is most likely too coarse for precise detector simulation.
In this module, the range cut-off is automatically calculated as a fifth of the minimal feature size of a single pixel, i.e. either to a fifth of the smallest pitch of a fifth of the sensor thickness, if smaller.
This behavior can be overwritten by explicitly specifying the range cut via the `range_cut` parameter.
The propagation of any particle is stopped at the value of the parameter `cutoff_time`. In case the particle is stopped in a sensitive volume, the remaining kinetic energy is deposited in this sensor.

The module supports the propagation of charged particles in a magnetic field if defined via the MagneticFieldReader module.

With the `output_plots` parameter activated, the module produces histograms of the total deposited charge per event for every sensor in units of kilo-electrons.
The scale of the plot axis can be adjusted using the `output_plots_scale` parameter and defaults to a maximum of 100ke.

### Dependencies

This module requires an installation Geant4.

### Parameters
* `physics_list`: Geant4-internal list of physical processes to simulate, defaults to FTFP_BERT_LIV. More information about possible physics list and recommendations for defaults are available on the Geant4 website [@g4physicslists].
* `enable_pai`: Determines if the Photoabsorption Ionization model is enabled in the sensors of all detectors. Defaults to false.
* `pai_model`: Model can be **pai** for the normal Photoabsorption Ionization model or **paiphoton** for the photon model. Default is **pai**. Only used if *enable_pai* is set to true.
* `charge_creation_energy` : Energy needed to create a charge deposit. Defaults to the energy needed to create an electron-hole pair in silicon (3.64 eV, [@chargecreation]).
* `fano_factor`: Fano factor to calculate fluctuations in the number of electron/hole pairs produced by a given energy deposition. Defaults to 0.115 [@fano].
* `max_step_length` : Maximum length of a simulation step in every sensitive device. Defaults to 1um.
* `range_cut` : Geant4 range cut-off threshold for the production of gammas, electrons and positrons to avoid infrared divergence. Defaults to a fifth of the shortest pixel feature, i.e. either pitch or thickness.
* `particle_type` : Type of the Geant4 particle to use in the source (string). Refer to the Geant4 documentation [@g4particles] for information about the available types of particles.
* `particle_code` : PDG code of the Geant4 particle to use in the source.
* `source_energy` : Mean energy of the generated particles.
* `source_energy_spread` : Energy spread of the source.
* `source_position` : Position of the particle source in the world geometry.
* `source_type` : Shape of the source: **beam** (default), **point**, **square**, **sphere**, **macro**.
* `file_name` : Name of the macro file (if source_type=**macro**).
* `cutoff_time` : Maximum lifetime of particles to be propagated in the simulation. This setting is passed to Geant4 as user limit and assigned to all sensitive volumes. Particles and decay products are only propagated and decayed up the this time limit and all remaining kinetic energy is deposited in the sensor it reached the time limit in. Defaults to 221s (to ensure proper gamma creation for the Cs137 decay).
Note: Neutrons have a lifetime of 882 seconds and will not be propagated in the simulation with the default `cutoff_time`.
* `number_of_particles` : Number of particles to generate in a single event. Defaults to one particle.
* `output_plots` : Enables output histograms to be be generated from the data in every step (slows down simulation considerably). Disabled by default.
* `output_plots_scale` : Set the x-axis scale of the output plot, defaults to 100ke.

#### Parameters for source `beam`
* `beam_size` : Width of the Gaussian beam profile.
* `beam_divergence` : Standard deviation of the particle angles in x and y from the particle beam
* `beam_direction` : Direction of the beam as a unit vector.

Please note that the old source parameters from version v1.1.2 and before (`beam_energy`, `beam_energy_spread` and `beam_position`) are still supported but it is recommended to use the new corresponding ones.

#### Parameters for source `square`
* `square_side` : Length of the square side.
* `square_angle` : Cone opening angle defining the maximum submission angle. Defaults to `180deg`, i.e. emission into one full hemisphere.

#### Parameters for source `sphere`
* `sphere_radius` : Radius of the sphere source (particles start only from the surface).
* `sphere_focus_point` : Focus point of the sphere source. If not specified, the radiation field is isotropic inside the sphere.

### Usage
A possible default configuration to use, simulating a beam of 120 GeV pions with a divergence in x, is the following:

```ini
[DepositionGeant4]
physics_list = FTFP_BERT_LIV
particle_type = "pi+"
source_energy = 120GeV
source_position = 0 0 -1mm
source_type = "beam"
beam_direction = 0 0 1
beam_divergence = 3mrad 0mrad
number_of_particles = 1
```

A radioactive point source of Iron-55 could be simulated by the following configuration:

```ini
[DepositionGeant4]
physics_list = FTFP_BERT_LIV
particle_type = "Fe55"
source_energy = 0eV
source_position = 0 0 -1mm
source_type = "point"
number_of_particles = 1
```

A Xenon-132 ion beam could be simulated using the following configuration:

```ini
[DepositionGeant4]
physics_list = FTFP_BERT_LIV
particle_type = "ion/54/132/0/0eV"
source_energy = 10MeV
source_position = 0 0 -1mm
source_type = "beam"
beam_direction = 0 0 1
number_of_particles = 1
```

[@g4physicslists]: http://geant4.cern.ch/support/proc_mod_catalog/physics_lists/referencePL.shtml
[@g4particles]: http://geant4-userdoc.web.cern.ch/geant4-userdoc/UsersGuides/ForApplicationDeveloper/html/TrackingAndPhysics/particle.html
[@g4radioactive]: https://doi.org/10.1109/TNS.2013.2270894
[@g4gps]:  http://geant4-userdoc.web.cern.ch/geant4-userdoc/UsersGuides/ForApplicationDeveloper/html/GettingStarted/generalParticleSource.html
[@pdg]: http://hepdata.cedar.ac.uk/lbl/2016/reviews/rpp2016-rev-monte-carlo-numbering.pdf
[@pai]: https://doi.org/10.1016/S0168-9002(00)00457-5
[@chargecreation]: https://doi.org/10.1103/PhysRevB.1.2945
[@fano]: https://doi.org/10.1103%2FPhysRevB.22.5565
