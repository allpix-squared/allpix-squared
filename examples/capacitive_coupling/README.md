---
# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Capacitive Coupling"
description: "Example for the CapacitiveTransfer module"
---

This folder contains example files and configuration for the CapacitiveTransfer module.

The `capacitive_coupling.conf` configuration file, as it is, simulates 6 FE-I4b planes (aligned as in a telescope) with a FE-I4b as a device-under-test (DUT) between the 3rd and 4th telescope planes.
This geometry is defined in the `ccpd_example_detector.conf` file.
The `SimpleTransfer` module is used for the telescope planes while the `CapacitiveTransfer` module is used for the DUT.
The DUT is simulated with specific angles, nominal and minimum gaps, obtained from real measurements.
The simulation results, regarding the DUT, should present a lower efficiency on the bottom left corner of the DUT due to the increasing gap between the pixels, towards this direction, and consequently a smaller coupling capacitance.
The coupling capacitance for each gap is retrieved from the `gap_scan_coupling_sim.root` ROOT file.
More information are provided in the `CapacitiveTransfer` module documentation.

The `capacitance_matrix.txt` file contains a generic relative coupling matrix (same as in the configuration file) that can be used to simulate the cross-coupling effects in parallel CCPDs assemblies.
More information on possible configurations of the `CapacitiveTransfer` module are provided in its documentation.
