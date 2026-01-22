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

This modules produces probability maps of charge carrier propagation which can be used as lookup tables in fast simulations.
For every voxel of the output map, the probability of a charge carrier being collected by any of the 5x5 pixels around its
starting position is encoded as probability. Any code reading this map can therefore use initial positions of charge carriers
or groups thereof, lookup the respective starting voxel and pick a final destination for the charge carriers by
randomly sampling the end point distribution.

The probability is normalized to the total number of charge carriers in this voxel. In case of a limited charge collection
efficiency, the sum of the resulting probability distribution does not have to be equal to one, but might be lower. It is
therefore also possible to simulate sensors with radiation-induced defects, charge carrier recombination, or other effects.

The input data to this module can come from any simulation chain, including realistic charge depositions, but it is optimally
paired e.g. with the *DepositionPointCharge* module which allows uniform sampling of an entire pixel cell.
The module requires both *DepositedCharge* and *PixelCharge* information to relate them to each other, and to determine the
initial charge distribution for every charge seen at a pixel.

The algorithm for generating the probability maps works as follows:

* The module loops over all available *DepositedCharge* objects in the event
  * For each of these objects, it searches for any *PixelCharge* object which contains this specific *DepositedCharge* in its Monte Carlo history
    * For any matching *PixelCharge* object, all corresponding *PropagatedCharge* objects which have contributed to the final signal are selected from the Monte Carlo history
    * The total charge which originated from the given *DepositedCharge* and ends on the given *PixelCharge* is calculated
    * The respective initial deposition position, final pixel index, and fraction of total charge is added to a lookup table
  * Any such lookup table is added to the global propagation map
* Finally, in the `finalize()` method the full propagation map is normalized by the number of contributions (number of lookup tables) added to each respective bin and the map is written to file.

The module prints a warning in case of low statistics or even an entirely empty bin encountered during the normalization step.

## Parameters

* `file_type`: Type of the output field file, either `APF` or `INIT`, for the binary or ASCII file format, respectively.
* `file_name`: Name of the data file to create, relative to the output directory of the framework. The file extension `.apf` or `.init` will be appended if not present, depending on the chosen file type.
* `bins`: Array with the number of bins for the three dimensions x, y and z.
* `field_mapping`: Description of the mapping of the output propagation lookup table onto the sensor or pixel cell. Possible values are `SENSOR` for
  sensor-wide mapping, `PIXEL_FULL`, indicating that the map spans the full 2D plane and the field is centered around the
  pixel center, `PIXEL_HALF_TOP` or `PIXEL_HALF_BOTTOM` indicating that the field only contains only one half-axis along `y`,
  `HALF_LEFT` or `HALF_RIGHT` indicating that the field only contains only one half-axis along `x`, or `PIXEL_QUADRANT_I`,
  `PIXEL_QUADRANT_II`, `PIXEL_QUADRANT_III`, `PIXEL_QUADRANT_IV` stating that the field only covers the respective quadrant
  of the 2D pixel plane. Defaults to `PIXEL_FULL`.
* `carrier_type`: Type of charge carrier type to build the propagation map for, either `ELECTRON` or `HOLE`.

## Usage

A valid configuration for this module is the following:

```ini
[PropagationMapWriter]
file_type = APF
file_name = "my_propagation_map"
bins = 20 20 100
field_mapping = PIXEL_FULL
carrier_type = ELECTRON
```
