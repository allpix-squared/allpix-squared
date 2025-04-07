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
ready for consumption with the [PixESL Pixel detector Electronic System Level framework](https://cern.ch/pixesl).

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

## Usage

The following snippet enables this module to write data into a file named "test.apx""

```ini
[PixESLWriter]
file_name = "test"
```
