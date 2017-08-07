## DepositionGeant4
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>)  
**Status**: Functional  
**Output**: DepositedCharge, MCParticle  

#### Description
Module that creates the deposits in the sensitive devices, wrapper around the Geant4 logic. Depends on a geometry construction in the GeometryBuilderGeant4 module. Initializes the physical processes to simulate and create a particle source that will generate particles in every event. For all particles passing the detectors in the geometry, the energy loss is converted into charge deposits for all steps (of customizable size) in the sensor. The information about the truth particle passage is also made available for later modules.

#### Parameters
* `physics_list`: Internal Geant4 list of physical processes to simulate. More information about possible physics list and recommendations for default is available [here](http://geant4.cern.ch/support/proc_mod_catalog/physics_lists/referencePL.shtml).
* `charge_creation_energy` : Energy needed to create a charge deposit. Defaults to the energy needed to create an electron-hole pair in silicon (3.64 eV).
* `max_step_length` : Maximum length of a simulation step in every sensitive device.
* `particle_position` : Position of the particle source in the world geometry.
* `particle_type` : Type of the Geant4 particle to use in the source. Refer to [this](http://geant4.cern.ch/G4UsersDocuments/UsersGuides/ForApplicationDeveloper/html/TrackingAndPhysics/particle.html) page for information about the available types of particles.
* `particle_radius_sigma` : Standard deviation of the radius from the particle source.
* `particle_direction` : Direction of the particle as a unit vector.
* `particle_energy` : Energy of the generated particle.
* `number_of_particles` : Number of particles to generate in a single event. Defaults to one particle.

#### Usage
A solid default configuration to use, simulating a test beam of 120 GeV pions, is the following:

```ini
[DepositionGeant4]
physics_list = QGSP_BERT
particle_type = "pi+"
particle_amount = 1 
particle_energy = 120GeV
particle_position = 0 0 -1mm
particle_direction = 0 0 1
```
