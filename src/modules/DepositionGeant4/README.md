## DepositionGeant4
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>)  
**Status**: Functional  
**Output**: DepositedCharge, MCParticle  

#### Description
Module which deposits charge carriers in the active volume of all detectors. It acts as wrapper around the Geant4 logic and depends on the global geometry constructed by the GeometryBuilderGeant4 module. It initializes the physical processes to simulate a particle source that will in every event generate a particle beam with certain parameters as explained below.

The particle type can be set via a string (particle_type) or by the respective PDG code (particle_code). Refer to [the Geant4 webpage](http://geant4.cern.ch/G4UsersDocuments/UsersGuides/ForApplicationDeveloper/html/TrackingAndPhysics/particle.html) for information about the available types of particles and [this pdf](http://pdg.lbl.gov/2002/montecarlorpp.pdf) for a list of the particles and PDG codes.

For all particles passing the detectors in the geometry, the energy loss is converted into charge carriers deposited in every steps of the Geant4 simulation of the passage (of customizable size). The information about the truth particle passage is also made available.

#### Dependencies

This module requires an installation Geant4.

#### Parameters
* `physics_list`: Geant4-internal list of physical processes to simulate. More information about possible physics list and recommendations for default is available [on the Geant4 website](http://geant4.cern.ch/support/proc_mod_catalog/physics_lists/referencePL.shtml).
* `charge_creation_energy` : Energy needed to create a charge deposit. Defaults to the energy needed to create an electron-hole pair in silicon (3.64 eV).
* `max_step_length` : Maximum length of a simulation step in every sensitive device.
* `particle_type` : Type of the Geant4 particle to use in the source (string).
* `particle_code` : PDG code of the Geant4 particle to use in the source.
* `beam_energy` : Mean energy of the generated particles.
* `beam_energy_spread` : Energy spread of the generated particle beam.
* `beam_position` : Position of the particle beam/source in the world geometry.
* `beam_size` : Width of the Gaussian beam profile.
* `beam_divergence` : Standard deviation of the particle angles in x and y from the particle beam
* `beam_direction` : Direction of the particle as a unit vector.
* `number_of_particles` : Number of particles to generate in a single event. Defaults to one particle.

#### Usage
A possible default configuration to use, simulating a beam of 120 GeV pions with a divergence in x, is the following:

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
