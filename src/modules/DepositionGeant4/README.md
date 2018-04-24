# DepositionGeant4
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>), Tobias Bisanz (<tobias.bisanz@phys.uni-goettingen.de>)  
**Status**: Functional  
**Output**: DepositedCharge, MCParticle, MCTrack

### Description
Module which deposits charge carriers in the active volume of all detectors. It acts as wrapper around the Geant4 logic and depends on the global geometry constructed by the GeometryBuilderGeant4 module. It initializes the physical processes to simulate a particle beam that will deposit charges in every event.

The particle type can be set via a string (particle_type) or by the respective PDG code (particle_code). Refer to the Geant4 webpage [@g4particles] for information about the available types of particles and the PDG particle code definition [@pdg] for a list of the available particles and PDG codes.

For all particles passing the sensitive device of the detectors, the energy loss is converted into deposited charge carriers in every step of the Geant4 simulation. Optionally, the Photoabsorption Ionization model (PAI) can be activated to improve the modeling of very thin sensors [@pai]. The information about the truth particle passage is also fully available, with every deposit linked to a MCParticle. Each trajectory which passes through at least one detector is also registered and stored as a global MCTrack. MCParticles are linked to their respectice tracks and each track is linked to its parent track, if available.

A range cut-off threshold for the production of gammas, electrons and positrons is necessary to avoid infrared divergence. By default, Geant4 sets this value to 700um or even 1mm, which is most likely too coarse for precise detector simulation. In this module, the range cut-off is automatically calculated as a fifth of the minimal feature size of a single pixel, i.e. either to a fifth of the smallest pitch of a fifth of the sensor thickness, if smaller. This behavior can be overwritten by explicitly specifying the range cut via the `range_cut` parameter.

The module supports the propagation of charged particles in a magnetic field if defined via the MagneticFieldReader module.

With the `output_plots` parameter activated, the module produces histograms of the total deposited charge per event for every sensor in units of kilo-electrons.
The scale of the plot axis can be adjusted using the `output_plots_scale` parameter and defaults to a maximum of 100ke.

### Dependencies

This module requires an installation Geant4.

### Parameters
* `physics_list`: Geant4-internal list of physical processes to simulate, defaults to FTFP_BERT_LIV. More information about possible physics list and recommendations for defaults are available on the Geant4 website [@g4physicslists].
* `enable_pai`: Determines if the Photoabsorption Ionization model is enabled in the sensors of all detectors. Defaults to false.
* `pai_model`: Model can be **pai** for the normal Photoabsorption Ionization model or **paiphoton** for the photon model. Default is **pai**. Only used if *enable_pai* is set to true.
* `charge_creation_energy` : Energy needed to create a charge deposit. Defaults to the energy needed to create an electron-hole pair in silicon (3.64 eV).
* `max_step_length` : Maximum length of a simulation step in every sensitive device. Defaults to 1um.
* `range_cut` : Geant4 range cut-off threshold for the production of gammas, electrons and positrons to avoid infrared divergence. Defaults to a fifth of the shortest pixel feature, i.e. either pitch or thickness.
* `particle_type` : Type of the Geant4 particle to use in the source (string). Refer to the Geant4 documentation [@g4particles] for information about the available types of particles.
* `particle_code` : PDG code of the Geant4 particle to use in the source.
* `beam_energy` : Mean energy of the generated particles.
* `beam_energy_spread` : Energy spread of the generated particle beam.
* `beam_position` : Position of the particle beam/source in the world geometry.
* `beam_size` : Width of the Gaussian beam profile.
* `beam_divergence` : Standard deviation of the particle angles in x and y from the particle beam
* `beam_direction` : Direction of the particle as a unit vector.
* `number_of_particles` : Number of particles to generate in a single event. Defaults to one particle.
* `output_plots` : Enables output histograms to be be generated from the data in every step (slows down simulation considerably). Disabled by default.
* `output_plots_scale` : Set the x-axis scale of the output plot, defaults to 100ke.

### Usage
A possible default configuration to use, simulating a beam of 120 GeV pions with a divergence in x, is the following:

```ini
[DepositionGeant4]
physics_list = FTFP_BERT_LIV
particle_type = "pi+"
beam_energy = 120GeV
beam_position = 0 0 -1mm
beam_direction = 0 0 1
beam_divergence = 3mrad 0mrad
number_of_particles = 1
```

[@g4physicslists]: http://geant4.cern.ch/support/proc_mod_catalog/physics_lists/referencePL.shtml
[@g4particles]: http://geant4.cern.ch/G4UsersDocuments/UsersGuides/ForApplicationDeveloper/html/TrackingAndPhysics/particle.html
[@pdg]: http://hepdata.cedar.ac.uk/lbl/2016/reviews/rpp2016-rev-monte-carlo-numbering.pdf
[@pai]: https://doi.org/10.1016/S0168-9002(00)00457-5
