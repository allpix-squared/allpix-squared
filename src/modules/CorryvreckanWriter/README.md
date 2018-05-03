# CorryvreckanWriter
**Maintainer**: Daniel Hynds (daniel.hynds@cern.ch)  
**Status**: Functional  
**Input**: PixelHit

### Description
Takes all digitised pixel hits and converts them into Corryvreckan pixel format. These are then written to an output file in the expected format to be read in by the reconstruction software. Will optionally write out the MC Truth information, storing the MC particle class from Corryvreckan.

This module writes output compatible with Corryvreckan 0.7 and later.

### Parameters
* `file_name` : Output filename (file extension `.root` will be appended if not present). Defaults to `corryvreckanOutput.root`
* `geometry_file` : Name of the output geometry file in the Corryvreckan format. Defaults to `corryvreckanGeometry.conf`
* `output_mctruth` : Flag to write out MCParticle information for each hit. Defaults to false.

### Usage
Typical usage is:

```ini
[CorryvreckanWriter]
file_name = corryvreckan
output_mctruth = true
```
