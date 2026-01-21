---
# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "PropagationMapWriter"
description: "Writer module to generate propagation maps"
module_status: "Functional"
module_maintainers: ["Simon Spannagel (<simon.spannagel@cern.ch>)"]
module_inputs: ["DepositedCharge", "PixelCharge"]
---

## Description

## Parameters

* `file_name`: Name of the data file to create, relative to the output directory of the framework. The file extension `.apf` will be appended if not present.
* `bins`: Array with the number of bins for the three dimensions x, y and z.
* `field_mapping`: Description of the mapping of the output propagation lookup table onto the sensor or pixel cell. Possible values are `SENSOR` for
  sensor-wide mapping, `PIXEL_FULL`, indicating that the map spans the full 2D plane and the field is centered around the
  pixel center, `PIXEL_HALF_TOP` or `PIXEL_HALF_BOTTOM` indicating that the field only contains only one half-axis along `y`,
  `HALF_LEFT` or `HALF_RIGHT` indicating that the field only contains only one half-axis along `x`, or `PIXEL_QUADRANT_I`,
  `PIXEL_QUADRANT_II`, `PIXEL_QUADRANT_III`, `PIXEL_QUADRANT_IV` stating that the field only covers the respective quadrant
  of the 2D pixel plane. Defaults to `PIXEL_FULL`.
* `carrier_type`: Type of charge carrier type to build the propagation map for, either `ELECTRON` or `HOLE`.

## Usage

Include an *example how to use this module*
