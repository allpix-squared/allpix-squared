---
# SPDX-FileCopyrightText: 2017-2023 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "LCIOWriter"
description: "Writes pixel hits to a LCIO file"
module_status: "Functional"
module_maintainers: ["Andreas Nurnberg (<andreas.nurnberg@cern.ch>)", "Simon Spannagel (<simon.spannagel@cern.ch>)", "Tobias Bisanz (<tobias.bisanz@phys.uni-goettingen.de>)"]
module_inputs: ["PixelHit"]
---

## Description
Writes pixel hit data to LCIO file, compatible with the EUTelescope analysis framework \[[@eutelescope]\].

If the `geometry_file` parameter is set to a non-empty string, a matching GEAR XML file is created from the simulated detector geometry and written to the simulation output directory. This GEAR file can be used with EUTelescope directly to reconstruct particle trajectories.

Optionally, if `dump_mc_truth` is set to true, this module will create Monte Carlo truth collections in the output LCIO file.

## Parameters
* `file_name`: name of the LCIO file to write, relative to the output directory of the framework. The extension **.slcio** should be added. Defaults to `output.slcio`.
* `geometry_file` : name of the output GEAR file to write the EUTelescope geometry description to. Defaults to `allpix_squared_gear.xml`
* `pixel_type`: EUtelescope pixel type to create. Options: EUTelSimpleSparsePixelDefault = 1, EUTelGenericSparsePixel = 2, EUTelTimepix3SparsePixel = 5 (Default: EUTelGenericSparsePixel)
* `detector_name`: Detector name written to the run header. Default: "EUTelescope"
* `dump_mc_truth`: Export the Monte Carlo truth data. Default: "false"

Only one of the following options must be used, if none is specified `output_collection_name` will be used with its default value.

* `output_collection_name`: Name of the LCIO collection containing the pixel data. Detectors will be assigned ascending sensor ids starting with 0. Default: "zsdata_m26"
* `detector_assignment`: A matrix with three entries each row: `["detector_name", "output_collection", "sensor_id"]`, one row for each detector. This allows to assign different output collections and sensor ids within the same set-up. `detector_name` is the detector's name as specified in the geometry file, `output_collection` the desired LCIO collection name and `sensor_id` the id used in the exported LCIO data. Sensor ids must be unique.

If only one detector is present in the `detector_assignment`, the value has to be encapsulated in extra quotes, i.e. `[["mydetector", "zsdata_test", "123"]]`.

## Usage
```ini
[LCIOWriter]
file_name = "run000123-converter.slcio"
```

Using the `detector_assignment` to write into two collections and assigning sensor id 20 to the device under test. Further, exporting the Monte Carlo truth data and writing the GEAR file:

```ini
[LCIOWriter]
file_name = "run000123-converter.slcio"
geometry_file = "run000123-gear.xml"
dump_mc_truth = true
detector_assignment = ["telescope0", "zsdata_m26", "0"], ["mydut", "zsdata_dut", "20"], ["telescope1", "zsdata_m26", "1"]
```

[@eutelescope]: https://eutelescope.github.io/
