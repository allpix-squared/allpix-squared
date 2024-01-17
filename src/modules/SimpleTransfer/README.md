---
# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "SimpleTransfer"
description: "Transfers propagated charges to the pixel front-end"
module_status: "Functional"
module_maintainers: ["Simon Spannagel (<simon.spannagel@cern.ch>)"]
module_inputs: ["PropagatedCharge"]
module_outputs: ["PixelCharge"]
---

## Description
Combines individual sets of propagated charges together to a set of charges on the sensor pixels and thus prepares them for processing by the detector front-end electronics. The module does a simple direct mapping to the nearest pixel, ignoring propagated charges that are too far away from the implants or outside the pixel grid. Timing information for the pixel charges is currently not yet produced, but can be fetched from the linked propagated charges.

If one or more implants are defined for the respective detector model, the `collect_from_implant` option can be turned on in order to only pick charge carriers from the implant volume and ignore everything outside this region.
Only charge carriers from front-side implants are collected, charge carriers on back-side implants are dropped.
Since this will lead to unexpected and undesired behavior when using linear electric fields, this option can only be used when using fields with an x/y dependence (i.e. field maps imported from TCAD).
In case no implants are defined, charge carriers are collected from the pixel surface and the parameter `max_depth_distance` can be used to control the depth from which charge carriers are taken into account.

A histogram of charge carrier arrival times is generated if `output_plots` is enabled. The range and granularity of this plot can be configured.

## Parameters
* `max_depth_distance` : Maximum distance in depth, i.e. normal to the sensor surface at the implant side, for a propagated charge to be taken into account in case the detector has no implants defined. Defaults to `5um`.
* `collect_from_implant`: Only consider charge carriers within the implant region of the respective detector instead of the full surface of the sensor. Should only be used with non-linear electric fields and defaults to `false`.
* `output_plots`: Determines if output plots should be generated. Disabled by default.
* `output_plots_step`: Bin size of the arrival time histogram in units of time. Defaults to `0.1ns`.
* `output_plots_range`: Total range of the arrival time histogram. Defaults to `100ns`.

## Usage
For a typical simulation, a *max_depth_distance* a few micro meters should be sufficient, leading to the following configuration:

```ini
[SimpleTransfer]
max_depth_distance = 5um
```
