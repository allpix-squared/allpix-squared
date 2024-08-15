---
# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "CapacitiveTransfer"
description: "Transfer with cross-coupling between pixels"
module_status: "Functional"
module_maintainers: ["Mateus Vicente (mvicente@cern.ch)"]
module_inputs: ["PropagatedCharge"]
module_outputs: ["PixelCharge"]
---

## Description
Similar to the SimpleTransferModule, this module combines individual sets of propagated charges together to a set of charges on the sensor pixels and thus prepares them for processing by the detector front-end electronics. In addition to the SimpleTransferModule, where the charge close to the implants is transferred only to the closest read-out pixel, this module also copies the propagated charge to the neighboring pixels, scaled by the respective cross-coupling (i.e. `cross_capacitance / nominal_capacitance`), in order to simulate the cross-coupling between neighboring pixels in Capacitively Coupled Pixel Detectors (CCPDs).

It is also possible to simulate assemblies with tilted chips, with non-uniform coupling over the pixel matrix, by providing the tilting angles between the chips, the nominal and minimum gaps between the pixel pads, the pixel coordinates where the chips are away from each other by the minimum gap provided and a root file containing ROOT::TGraph with coupling capacitances *vs* gap between pixel pads.

The coupling matrix (imported via the `coupling_matrix` or the `coupling_file` configuration keys) represents the pixels coupling with a nominal gap between the chips, while the ROOT file imported with the configuration key `coupling_scan_file` contains the coupling between the pixels for several gaps.

The coupling matrices can be used to easily simulate the cross-coupling in CCPDs with the nominal, and constant, gap between chips over the pixel matrix.
In such cases, the "central pixel" (center element of the coupling matrix) always receive 100% of the charge transferred while neighbor pixels, with lower coupling capacitance, gets a fraction of the charged transferred to the central pixel, normalized by the nominal capacitance (capacitance to central pixel).
The coupling matrices always represents the coupling in fractions from 0 (no charge transferred) up to 1 (100% transfer).

If a coupling_scan_file is provided the gap between the chips will be calculated on each pixel with a hit and the charge transferred will be normalized by the capacitance value of the central pixel at the nominal gap.
This model will reproduce the results with the coupling matrices if `chip_angle = 0rad 0rad` (parallel chips) and `minimum_gap = nominal_gap`.

## Dependencies

This module requires an installation of Eigen3.

## Parameters
* `coupling_scan_file`: Root file containing a TGraph, for each pixel, with the capacitance simulated for each gap between the pixel pads. The TGraph objects in the root file should be named `Pixel_X` where `X` goes from 1 to 9.
* `chip_angle`: Tilt angle between chips. The first angle is the rotation along the columns axis,  and second is along the row axis. It defaults to 0.0 radians (parallel chips).
* `tilt_center`: Pixel position for the nominal coupling/distance.
* `nominal_gap`: Nominal gap between chips.
* `minimum_gap`: Closest distance between chips.
* `cross_coupling`: Enables cross-coupling between pixels. Defaults to `true` (enabled).
* `coupling_file`: Path to the file containing the cross-coupling matrix. The file must contain the relative capacitance to the central pixel.
* `coupling_matrix`: Cross-coupling matrix with relative capacitances.
* `max_depth_distance`: Maximum distance in depth, i.e. normal to the sensor surface at the implant side, for a propagated charge to be taken into account in case the detector has no implants defined or `collect_from_implant` is set to `false`. Defaults to `5um`.
* `collect_from_implant`: Only consider charge carriers within the implant region of the respective detector instead of the full surface of the sensor. Should only be used with non-linear electric fields and defaults to `false`.
* `output_plots`: Saves the output plots for this module. Defaults to 1 (enabled).

The cross-coupling matrix, to be parsed via the matrix file or via the configuration file, must be organized in Row vs Col, such as:

```ini
 cross_coupling_00    cross_coupling_01    cross_coupling_02
 cross_coupling_10    cross_coupling_11    cross_coupling_12
 cross_coupling_20    cross_coupling_21    cross_coupling_22
```

The matrix center element, `cross_coupling_11` in this example, is the coupling to the closest pixel and should be always 1.
The matrix can have any size, although square 3x3 matrices are recommended as the coupling decreases significantly after the first neighbors and the simulation will scale with NxM, where N and M are the respective sizes of the matrix.

## Usage
This module accepts only one coupling model (`coupling_scan_file`, coupling_file or `coupling_matrix`) at each time. If more then one option is provided, the simulation will not run.

```ini
[CapacitiveTransfer]
coupling_scan_file = "capacitance_vs_gap.root"
nominal_gap = 2um
minimum_gap = 8um
chip_angle = -0.000524rad 0.000350rad
tilt_center = 80 336
cross_coupling = true
max_depth_distance = 5um
```

or

```ini
[CapacitiveTransfer]
max_depth_distance = 5um
coupling_file = "capacitance_matrix.txt"
```

or

```ini
[CapacitiveTransfer]
max_depth_distance = 5um
coupling_matrix = [[0.1, 0.3, 0.1], [0.2, 1, 0.2], [0.1, 0.3, 1.1]]
```
