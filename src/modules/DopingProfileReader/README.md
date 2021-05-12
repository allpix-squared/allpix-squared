# DopingConcentrationReader
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch)  
**Status**: Functional

### Description
Adds a doping profile to the detector from one of the supported sources. By default, detectors do not have a doping profile applied.
A doping profile is required for simulating the lifetime of charge carriers.
It is not used for the calculation of the electric field inside the sensor.
The profile is extrapolated along `z` such that if a position outside the sensor is queried, the last value available at the sensor surface is returned.
This precludes edge effects from charge carriers moving at the sensor surfaces.

The following models for the doping profile can be used:

* For **constant**, a constant doping profile is set in the sensor
* For **regions**, the sensor is segmented into slices along the local z-direction. In each slice, a constant doping concentration is used. The user provides the depth of each slice and the corresponding concentration.
* For **mesh**, a file containing a doping profile map in APF or INIT format is parsed.

### Parameters
* `model` : Type of the doping profile, either **constant**, **regions**  or **mesh**.
* `file_name` : Location of file containing the doping profile in one of the supported field file formats.
Only used if the *model* parameter has the value **mesh**.
* `field_scale` :  Scale of the doping profile in x- and y-direction in units of pixels.
Only used if the *model* parameter has the value **mesh**.
* `field_offset` : Offset of the doping file from the pixel edge in x- and y-direction in units of pixels.
Only used if the *model* parameter has the value **mesh**.
* `doping_concentration` : Value for the doping concentration. If the *model* parameter has the value **constant** a single number should be provided. If the *model* parameter has the value **regions** a matrix is expected, which provides the sensor depth and doping concentration in each row.
* `doping_depth` : Thickness of the doping profile region. The doping profile is extrapolated in the region below the `doping_depth`.
Only used if the *model* parameter has the value **mesh**.

### Usage
```toml
[DopingProfileReader]
model = "mesh"
file_name = "example_doping_profile.apf"
```
