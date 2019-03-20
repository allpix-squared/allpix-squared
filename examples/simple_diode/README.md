# Am241 Source Measurement with a  paper collimator

This example simulates an Am241 alpha source using Geant4's a native GPS macro. The Source is defined as a disk from which is emmited monoenergetic 5.4 MeV alphas which approximate Am241 alpha spectrum. Alphas are emmited isotropically.

A diode-type detector is placed below the source, shielded with an additional sheet of paper of 200um with a pinhole in it to let the alpha pass. The idea is to reproduce the aperture effect with alpha, where the detected spectrum show a dependency on pinhole size due to the different path length of the alpha particles in the air as function of the incident angle. 

The charge deposition is performed by Geant4 using a standard physics list and a stepping length of 10um.
The `ProjectionPropagation` module with a setting of `charge_per_step = 500` is used to simulate the charge carrier propagation and the simulation result is stored to file.


![diode display](diode_display.png)