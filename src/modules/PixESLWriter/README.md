---
# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "PixESLWriter"
description: "Write simulation data for the PixESL Framework"
module_status: "Functional"
module_maintainers: ["Simon Spannagel (<simon.spannagel@cern.ch>)"]
module_inputs: ["PixelHit"]
---

## Description

This detector module writes output files in the [Allpix Squared PixESL Exchange Format](https://gitlab.cern.ch/allpix-squared/libapx)
ready for consumption with the [PixESL Pixel detector Electronic System Level framework](https://cern.ch/pixesl) and requires an installation of `libAPX`.

This module consumes `PixelHit` objects and and writes the following record properties to the output file:

* `column` (integer value)
* `row` (integer value)
* `charge` (floating-point value)
* `toa` (floating-point value)

It should be noted that Allpix Squared only calls this module if the respective detector has actually received a pixel hit.
If e.g. all charge remains below a previously imposed threshold, this module will not be called and the resulting APX output
file will not contain an event entry with the respective event ID.

## Parameters

* `file_name`: Name of the output file. The file will automatically placed in a subdirectory for this detector and appended
  with the `.apx` file extension.
* `BX_period` : Bunch cross period. Cyclic duration to reproduce ina simple way a bunch cross behavior.
* `mean_hit_rate` : Mean hit rate, used to calculate the timestamps of the events for the `warm_up_duration` and in the BX cycles after the end of the `peak_duration` before starting a new cycle. No default value.
* `peak_hit_rate` : Optional, this value is used to create timestamps with a different hit rate in a defined duration `peak_duration`, no default value.
* `peak_duration` : Optional, to define the duration of the `peak_hit_rate`, no default value.
* `warm_up_duration` : Optional, duration before starting the BX cycles. The hit rate for this duration is set to the `mean_hit_rate`.

## Usage

The following snippet enables this module to write data into a file named "test.apx""

```ini
[PixESLWriter]
file_name = "test"
```
