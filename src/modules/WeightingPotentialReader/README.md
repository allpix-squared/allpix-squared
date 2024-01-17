---
# SPDX-FileCopyrightText: 2019-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "WeightingPotentialReader"
description: "Weighting potential loading for a detector"
module_status: "Functional"
module_maintainers: ["Simon Spannagel (<simon.spannagel@cern.ch>)"]
---

## Description
Adds a weighting potential (Ramo potential) to the detector from one of the supported sources. By default, detectors do not have a weighting potential applied.
This module support two types of weighting potentials.

### Weighting potential map

Using the **mesh** model of this module allows reading in from a file, e.g. from an electrostatic TCAD simulation.
A converter tool for fields from adaptive TCAD meshes is provided with the framework.
The map is expected to be symmetric around the reference pixel the weighting potential is calculated for, the size of the field is taken from the file header.

The potential field map needs to be three-dimensional.
Otherwise the induced current on neighboring pixels along the missing component will always be exactly the same as the actual pixel under which the charge is present because the same weighting potential is samples - with a two-dimensional field, distances in the third dimension are always zero.
This will lead to unphysical results and a multiplication of the total charge.
If this behavior is desirable, or e.g. only a single row of pixels is simulated, the check can be omitted by setting `ignore_field_dimensions = true`.

A warning is printed if the size does not correspond to a multiple of the pixel size.
While this is not a problem in general, it might hint at a wrong potential map being used.

#### Generating a weighting potential using TCAD and Allpix Squared

The weighting potential is calculated by taking the difference of the electrostatic potentials arising from applying two slightly different bias voltages to one electrode.
The steps below outline how to create a weighting potential from TCAD simulations.

1. Produce two TCAD fields that differ slightly in one collection electrode bias voltage, for instance for 0.1 V or 0.01 V, with all the other electrodes grounded. Export the two resulting electrostatic potentials into separate files.
2. Use the `mesh_converter` tool to extract the electrostatic potential from both configurations. Working with the converted files in INIT format is advisable as is human readable and this makes the process of writing a macro for the calculation simpler.
3. Calculate the difference between entries from both files, and divide them by the difference in collection electrode bias voltage in order to normalize them to the range `[0, 1]`.
4. Verify that the values are within a range from 0 to 1, which is the physical range of a weighting potential.
5. Save the resulting file with the same format and import it into Allpix Squared using the `[WeightingPotentialReader]` module and the **mesh** model.

### Weighting potential of a pad

When setting the **pad** model, the weighting potential of a pixel in a plane condenser is calculated numerically from first principles, following the procedure described in detail in \[[@planecondenser]\].
It should be noted that this calculation is comparatively **slow and takes about a factor 100 longer** than a lookup from a pre-calculated field map.
A tool to generate the field map using the method described herein is provided in the software repository.

The weighting potential is calculated via Green's reciprocity theorem, the integral part of the expression are ignored.
In \[[@planecondenser]\] it has been shown that the uncertainty on the weighting potential is smaller than

$`| \Delta \phi_w | < \frac{V_w}{8\pi} \frac{w_x w_y}{d^2} \frac{1}{N^2} \frac{z}{d}`$,

where *N* limits the expansion of the series.
In this implementation, a value of $`N = 100`$ is used.
Following these calculations, the weighting potential is given by

$`\phi_w/V_w = \frac{1}{2\pi}f(x, y, z) - \frac{1}{2\pi}\sum_{n = 1}^N\left[f(x, y, 2nd - z) - g_z(x, y, 2nd + z)\right] `$

with

$` f(x, y, u) = \arctan\left( \frac{x_1 y_1}{u\sqrt{x_1^2 + y_1^2 + u^2}} \right) + \arctan\left( \frac{x_2 y_2}{u\sqrt{x_2^2 + y_2^2 + u^2}} \right) - \arctan\left( \frac{x_1 y_2}{u\sqrt{x_1^2 + y_2^2 + u^2}} \right) - \arctan\left( \frac{x_2 y_1}{u\sqrt{x_2^2 + y_1^2 + u^2}} \right)`$

with $`x_{1,2} = x \pm \frac{w_x}{2} \qquad y_{1,2} = y \pm \frac{w_y}{2}`$. The parameters $`w_{x,y}`$ indicate the size of the collection electrode (i.e. the implant), $`V_w`$ is the potential of the electrode and *d* is the thickness of the sensor.


## Parameters
- `model` : Type of the weighting potential model, either **mesh** or **pad**.
- `file_name` : Location of file containing the weighting potential in one of the supported field file formats. Only used if
  the *model* parameter has the value **mesh**.
- `field_mapping`: Description of the mapping of the field onto the sensor or pixel cell. Possible values are `PIXEL_FULL`,
  indicating that the map spans the full 2D plane and the field is centered around the pixel center, `PIXEL_HALF_TOP` or
  `PIXEL_HALF_BOTTOM` indicating that the field only contains only one half-axis along `y`, `HALF_LEFT` or `HALF_RIGHT`
  indicating that the field only contains only one half-axis along `x`, or `PIXEL_QUADRANT_I`, `PIXEL_QUADRANT_II`,
  `PIXEL_QUADRANT_III`, `PIXEL_QUADRANT_IV` stating that the field only covers the respective quadrant of the 2D pixel
  plane. In addition, the `PIXEL_FULL_INVERSE` mode allows loading full-plane field maps which are not centered around a
  pixel cell but the corner between pixels. Only used if the *model* parameter has the value **mesh**.
- `field_scale`:  Scaling factor of the weighting potential in x- and y-direction. By default, the scaling factors are set to
  `{1, 1}` and the field is used with its physical extent stated in the field data file.
- `potential_depth` : Thickness of the weighting potential region. The weighting potential is set to zero in the region below the
  `potential_depth`. Defaults to the full sensor thickness. Only used if the *model* parameter has the value **mesh**.
- `ignore_field_dimensions`: If set to true, a wrong dimensionality of the input field is ignored, otherwise an exception is
  thrown. Defaults to false.
- `output_plots`:  Determines if output plots should be generated. Disabled by default.
- `output_plots_steps` : Number of bins along the z-direction for which the weighting potential is evaluated. Defaults to
  500 bins and is only used if `output_plots` is enabled.
- `output_plots_position`: 2D Position in x and y at which the weighting potential is evaluated along the z-axis. By default,
  the potential is plotted for the position in the pixel center, i.e. (0, 0). This parameter only affects the 1D weighting
  potential histogram. Only used if `output_plots` is enabled.
- `output_plots_zcut`: Position along the sensor `z` axis at which the 2D `x`-`y` weighting potential profile is evaluated.
  Defaults to `0um`, i.e. the center plane of the sensor.

## Usage
An example to add a weighting potential form a field data file to the detector called "dut" is given below.

```ini
[WeightingPotentialReader]
name = "dut"
model = "mesh"
file_name = "example_weighting_field.apf"
```

[@planecondenser]: https://doi.org/10.1016/j.nima.2014.08.044
