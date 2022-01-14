<!--
SPDX-FileCopyrightText: 2019-2022 CERN and the Allpix Squared authors
SPDX-License-Identifier: CC-BY-4.0
-->

# WeightingPotentialReader
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Functional

### Description
Adds a weighting potential (Ramo potential) to the detector from one of the supported sources. By default, detectors do not have a weighting potential applied.
This module support two types of weighting potentials.

#### Weighting potential map

Using the **mesh** model of this module allows reading in from a file, e.g. from an electrostatic TCAD simulation.
A converter tool for fields from adaptive TCAD meshes is provided with the framework.
The map is expected to be symmetric around the reference pixel the weighting potential is calculated for, the size of the field is taken from the file header.

The potential field map needs to be three-dimensional.
Otherwise the induced current on neighboring pixels along the missing component will always be exactly the same as the actual pixel under which the charge is present because the same weighting potential is samples - with a two-dimensional field, distances in the third dimension are always zero.
This will lead to unphysical results and a multiplication of the total charge.
If this behavior is desirable, or e.g. only a single row of pixels is simulated, the check can be omitted by setting `ignore_field_dimensions = true`.

A warning is printed if the size does not correspond to a multiple of the pixel size.
While this is not a problem in general, it might hint at a wrong potential map being used.

#### Weighting potential of a pad

When setting the **pad** model, the weighting potential of a pixel in a plane condenser is calculated numerically from first principles, following the procedure described in detail in [@planecondenser].
It should be noted that this calculation is comparatively **slow and takes about a factor 100 longer** than a lookup from a pre-calculated field map.
A tool to generate the field map using the method described herein is provided in the software repository.

The weighting potential is calculated via Green's reciprocity theorem, the integral part of the expression are ignored.
In [@planecondenser] it has been shown that the uncertainty on the weighting potential is smaller than

$`| \Delta \phi_w | < \frac{V_w}{8\pi} \frac{w_x w_y}{d^2} \frac{1}{N^2} \frac{z}{d}`$,

where *N* limits the expansion of the series.
In this implementation, a value of $`N = 100`$ is used.
Following these calculations, the weighting potential is given by

$`\phi_w/V_w = \frac{1}{2\pi}f(x, y, z) - \frac{1}{2\pi}\sum_{n = 1}^N\left[f(x, y, 2nd - z) - g_z(x, y, 2nd + z)\right] `$

with

$` f(x, y, u) = \arctan\left( \frac{x_1 y_1}{u\sqrt{x_1^2 + y_1^2 + u^2}} \right) + \arctan\left( \frac{x_2 y_2}{u\sqrt{x_2^2 + y_2^2 + u^2}} \right) - \arctan\left( \frac{x_1 y_2}{u\sqrt{x_1^2 + y_2^2 + u^2}} \right) - \arctan\left( \frac{x_2 y_1}{u\sqrt{x_2^2 + y_1^2 + u^2}} \right)`$

with $`x_{1,2} = x \pm \frac{w_x}{2} \qquad y_{1,2} = y \pm \frac{w_y}{2}`$. The parameters $`w_{x,y}`$ indicate the size of the collection electrode (i.e. the implant), $`V_w`$ is the potential of the electrode and *d* is the thickness of the sensor.


### Parameters
* `model` : Type of the weighting potential model, either **mesh** or **pad**.
* `file_name` : Location of file containing the weighting potential in one of the supported field file formats. Only used if the *model* parameter has the value **mesh**.
* `ignore_field_dimensions`: If set to true, a wrong dimensionality of the input field is ignored, otherwise an exception is thrown. Defaults to false.
* `output_plots`:  Determines if output plots should be generated. Disabled by default.
* `output_plots_steps` : Number of bins along the z-direction for which the weighting potential is evaluated. Defaults to 500 bins and is only used if `output_plots` is enabled.
* `output_plots_position`: 2D Position in x and y at which the weighting potential is evaluated along the z-axis. By default, the potential is plotted for the position in the pixel center, i.e. (0, 0). Only used if `output_plots` is enabled.

### Usage
An example to add a weighting potential form a field data file to the detector called "dut" is given below.

```ini
[WeightingPotentialReader]
name = "dut"
model = "mesh"
file_name = "example_weighting_field.apf"
```

[@planecondenser]: https://doi.org/10.1016/j.nima.2014.08.044
