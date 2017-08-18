## DepositionGeant4
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>)  
**Status**: Functional  
**Output**: DepositedCharge, MCParticle  

#### Description
Module which deposits charge carriers in the active volume of all detectors. It acts as wrapper around the Geant4 logic and depends on the global geometry constructed by the GeometryBuilderGeant4 module. It initializes the physical processes to simulate a particle source that will generate particles in every event. For all particles passing the detectors in the geometry, the energy loss is converted into charge carrier deposited in every steps of the Geant4 simulation of the passage (of customizable size). The information about the truth particle passage is also made available.

#### Dependencies

This module requires an installation Geant4.

#### Parameters
* `physics_list`: Geant4-internal list of physical processes to simulate. More information about possible physics list and recommendations for default is available [on the Geant4 website](http://geant4.cern.ch/support/proc_mod_catalog/physics_lists/referencePL.shtml).
* `charge_creation_energy` : Energy needed to create a charge deposit. Defaults to the energy needed to create an electron-hole pair in silicon (3.64 eV).
* `max_step_length` : Maximum length of a simulation step in every sensitive device.
* `particle_position` : Position of the particle source in the world geometry.
* `particle_type` : Type of the Geant4 particle to use in the source. Refer to [the Geant4 documentation](http://geant4.cern.ch/G4UsersDocuments/UsersGuides/ForApplicationDeveloper/html/TrackingAndPhysics/particle.html) page for information about the available types of particles.
* `particle_radius_sigma` : Width of the Gaussian beam profile.
* `particle_direction` : Direction of the particle as a unit vector.
* `particle_energy` : Initial energy of the generated particle.
* `number_of_particles` : Number of particles to generate in a single event. Defaults to one particle.

#### Usage
A possible default configuration to use, simulating a beam of 120 GeV pions, is the following:

```ini
[DepositionGeant4]
physics_list = QGSP_BERT
particle_type = "pi+"
particle_energy = 120GeV
particle_position = 0 0 -1mm
particle_direction = 0 0 1
number_of_particles = 1
```
