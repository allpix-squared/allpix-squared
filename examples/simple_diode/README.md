# Am241 Source Measurement with a Paper Collimator over a Diode

This example simulates an Am241 alpha source using a native Geant4 GPS macro. The source is defined as a disk from which mono-energetic 5.4 MeV alphas are emitted. This approximates the Am241 alpha spectrum. The source emits the alpha particles isotropically.

A diode-type detector is placed below the source, shielded with an additional sheet of paper of 200um with a pinhole in it to let the alpha pass. The idea is to reproduce the aperture effect with alpha, where the detected spectrum show a dependency on pinhole size due to the different path length of the alpha particles in the air as function of the incident angle.

The charge deposition is performed by Geant4 using a standard physics list and a stepping length of 10um.
The `ProjectionPropagation` module with a setting of `charge_per_step = 500` is used to simulate the charge carrier propagation and the simulation result is stored to file. The `model_paths` parameter is set to add this directory to the search path for detector models.
