# Source Measurement with Shielding

This example simulates an (admittedly not very physical) source emitting electrons with Gaussian distributed energy of $`N(0.56MeV, 0.20MeV)`$.
A Medipix-type detector is placed below the source, shielded with an additional sheet of aluminum with a thickness of 8mm.
No misalignment is added but the absolute position and orientation of the detector is specified.

The setup of the simulation chain follows the "fast simulation example:
The charge deposition is performed by Geant4 using a standard physics list and a stepping length of 10um.
The `ProjectionPropagation` module with a setting of `charge_per_step = 100` is used to simulate the charge carrier propagation and the simulation result is stored to file excluding `DepositedCharge` and `PropagatedCharge` objects to keep the output file size low.
