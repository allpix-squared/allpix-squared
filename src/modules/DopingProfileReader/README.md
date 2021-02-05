# DopingConcentrationReader
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch)  
**Status**: Immature

### Description
Adds a doping profile to the detector from one of the supported sources. By default, detectors do not have a doping profile applied.
A doping profile is required for simulating the lifetime of charge carriers. 

The following models for the doping profile can be used:

* For **constant**, a constant doping profile is set in the sensor
* For **regions**, the sensor is segmented into slices along the local z-direction. In each slice, a constant doping concentration is used. The user provides the depth of each slice and the corresponding concentration.
* For **mesh**, a file containing a doping profile map in APF or INIT format is parsed. 

### Parameters
* `model` : Type of the weighting potential model, either **constant**, **regions**  or **mesh**.
* `file_name` : Location of file containing the doping profile in one of the supported field file formats. 
Only used if the *model* parameter has the value **mesh**.
* `field_scale` :  Scale of the doping profile in x- and y-direction. 
Only used if the *model* parameter has the value **mesh**.
* `field_offset` : Offset of the doping file from the pixel edge in x- and y-direction. 
Only used if the *model* parameter has the value **mesh**.
* `doping_concentration` : Value for the doping concentration. If the *model* parameter has the value **constant** a single number should be provided. If the *model* parameter has the value **regions** a matrix is expected, which provides the sensor depth and doping concentration in each row.
* `doping_depth` : Thickness of the doping profile region. 
Only used if the *model* parameter has the value **mesh**.

### Usage
```toml
[DopingProfileReader]
model = mesh
file_name = "example_doping_profile.apf"
```
