## DepositionGeant4
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>)  
**Status**: Functional  
**Output**: DepositedCharge, MCParticle  

#### Description
Module that creates the deposits in the sensitive devices, wrapper around the Geant4 logic. Depends on a geometry construction in the GeometryBuilderGeant4 module. Initializes the physical processes to simulate and create a particle source that will generate particles in every event. This particle source can be called with various parameters as seen below, in order to set the particle type and the beam position, energy, energy spread, width and divergence.

The particle type can be set via a string (particle_type) or by the regarding PDG code (particle_code). Refer to [this](http://geant4.cern.ch/G4UsersDocuments/UsersGuides/ForApplicationDeveloper/html/TrackingAndPhysics/particle.html) page for information about the available types of particles and [this](http://pdg.lbl.gov/2002/montecarlorpp.pdf) pdf for a list of the particles and PDG codes.

For all particles passing the detectors in the geometry, the energy loss is converted into charge deposits for all steps (of customizable size) in the sensor. The information about the truth particle passage is also made available for later modules.

#### Parameters
* `physics_list`: Internal Geant4 list of physical processes to simulate. More information about possible physics list and recommendations for default is available [here](http://geant4.cern.ch/support/proc_mod_catalog/physics_lists/referencePL.shtml).
* `charge_creation_energy` : Energy needed to create a charge deposit. Defaults to the energy needed to create an electron-hole pair in silicon (3.64 eV).
* `max_step_length` : Maximum length of a simulation step in every sensitive device.
* `particle_type` : Type of the Geant4 particle to use in the source (string).
* `particle_code` : PDG code of the Geant4 particle to use in the source.
* `beam_energy` : Mean energy of the generated particle.
* `beam_energy_spread` : Energy spread of the generated particle beam.
* `beam_position` : Position of the particle beam/source in the world geometry.
* `beam_size` : Standard deviation of the radius from the particle beam.
* `beam_divergence` : Standard deviation of the particle angles in x and y from the particle beam
* `beam_direction` : Direction of the particle as a unit vector.
* `number_of_particles` : Number of particles to generate in a single event. Defaults to one particle.

#### Usage
A solid default configuration to use, simulating a test beam of 120 GeV pions with a divergence in x, is the following:

```ini
[DepositionGeant4]
physics_list = QGSP_BERT
particle_type = "pi+"
beam_energy = 120GeV
beam_position = 0 0 -1mm
beam_direction = 0 0 1
beam_divergence = 3mrad 0mrad
number_of_particles = 1
```
