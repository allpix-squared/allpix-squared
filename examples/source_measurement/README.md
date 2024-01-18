---
# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Source Measurement"
description: "Iron-55 source measurement with shielding"
---

This example simulates an Iron-55 source using Geant4's radioactive decay simulation. The particle type is set to `Fe55` to use the isotope, the source energy configured as `0eV` for a decay in rest. A point-like particle source is used.

A Medipix-type detector is placed below the source, shielded with an additional sheet of aluminum with a thickness of 8mm.
No misalignment is added but the absolute position and orientation of the detector is specified.

The setup of the simulation chain follows the "fast simulation example:
The charge deposition is performed by Geant4 using a standard physics list and a stepping length of 10um.
The `ProjectionPropagation` module with a setting of `charge_per_step = 100` is used to simulate the charge carrier propagation and the simulation result is stored to file excluding `DepositedCharge` and `PropagatedCharge` objects to keep the output file size low.
