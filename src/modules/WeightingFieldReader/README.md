# WeightingFieldReader
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Development

### Description
Adds a weighting field (Ramo field) to the detector from one of the supported sources. By default, detectors do not have a weighting field applied.

This module currently is work in progress and is not in a working state.

### Parameters
* `model` : Type of the weighting field model, currently only **init**.
* `file_name` : Location of file containing the weighting field in the INIT format. Only used if the *model* parameter has the value **init**.

### Usage
An example to add a weighting field via an INIT file to the detector called "dut" is given below.

```ini
[WeightingFieldReader]
name = "dut"
model = "init"
file_name = "example_weighting_field.init"
```
