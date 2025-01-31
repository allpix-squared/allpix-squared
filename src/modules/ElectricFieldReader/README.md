---
# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "ElectricFieldReader"
description: "Electric field loading for a detector"
module_status: "Functional"
module_maintainers: ["Simon Spannagel (<simon.spannagel@cern.ch>)"]
---

## Description

Adds an electric field to the detector from one of the supported sources. By default, detectors do not have an electric field
applied.

The reader provides the following models for electric fields:

* For **constant** electric fields it add a constant electric field in the z-direction towards the pixel implants. This is
  not physical but might aid in developing and testing new charge propagation algorithms.

* For **linear** electric fields, the field has a constant slope determined by the bias voltage and the depletion voltage.
  The sensor is depleted either from the implant or the back side, the direction of the electric field depends on the sign of
  the bias voltage (with negative bias voltage the electric field vector points towards the backplane and vice versa).
  The sign of depletion voltage is always ignored.
  If the sensor is depleted from the implant side, the absolute value of the electric field is calculated using the formula

  ```math
  E(z) = \frac{|U_{bias}| - |U_{depl}|}{d} + 2 \frac{|U_{depl}|}{d}\left( 1- \frac{z}{d} \right),
  ```

  where d is the thickness of the sensor, and $`U_{depl}`$, $`U_{bias}`$ are the depletion and bias voltages, respectively.
  In case of a depletion from the back side, the absolute value of the electric field is calculated as

  ```math
  E(z) = \frac{|U_{bias}| - |U_{depl}|}{d} + 2 \frac{|U_{depl}|}{d}\left( \frac{z}{d} \right).
  ```

* For **parabolic** electric fields, a parabola is defined in order to emulate a double-peaked field such as the electric
  fields observed in sensors after irradiation. The parabola is calculated from the position $`z_{min}`$ and value
  $`E_{min}`$ of the minimum field in the sensor and the field value at the readout electrode, $`E_{max}`$. The parameters of
  parabolic equation $`E(z) = az^2 + bz + c`$ then resolve to:

  ```math
  a = \frac{E_{max} - E_{min}}{z_{min}^2 + (d/2)^2 - dz_{min}} \qquad
  b = -2az_{min} \qquad
  c = E_{max} - a ((d/2)^2 - dz_{min}),
  ```

  where $`d`$ is the sensor thickness and $`z`$ the position along the `z`-axis in local coordinates, from $`-d/2`$ to
  $`+d/2`$. The direction of the electric field is determined by the sign of the field parameters.

* For electric fields from **mesh files** in the *INIT* or *APF* formats it parses a file containing an electric field map in
  the APF format or the legacy INIT format also used by the PixelAV software \[[@pixelav]\]. An example of a electric field
  in this format can be found in *etc/example_electric_field.init* in the repository. An explanation of the format is
  available in the source code of this module, a converter tool for electric fields from adaptive TCAD meshes is provided
  with the framework. Fields of different sizes can be used and mapped onto the pixel matrix using the `field_scale`
  parameter. By default, the module reads the size of the field from the file. If the field size and pixel pitch do not match,
  a warning is printed. The units in which the field itself is interpreted can be configured via the `file_units` parameter,
  by default the field values are interpreted as representing `V/cm`. This is also the unit used in the output files from the `mesh converter` tool.

* The **custom** field model allows to specify arbitrary analytic field functions for a single or all three vector components
  of the electric field. For this, the `field_functions` parameter configured with either one formula which is then used for
  the `z` component of the field vector, or with three functions representing the three components of the field vector. Using
  the `field_parameters` configuration, values for the free parameters defined in the formulae can be set. For the parameters
  units are supported and parsed. Each of the field vector components has access to its own free parameters as well as all
  three coordinates `x`, `y` and `z` which are defined as the position within the respective pixel.


The `depletion_depth` parameter can be used to control the thickness of the depleted region inside the sensor.
This can be useful for devices such as HV-CMOS sensors, where the typical depletion depth but not necessarily the full
depletion voltage are know. It should be noted that `depletion_voltage` and `depletion_depth` are mutually exclusive
parameters and only one at a time can be specified. The alias `field_depth` can be used instead, as this parameter is the
depth that the field will be created over. If the parameter is smaller than the field from an imported mesh, the field will
be compressed in the z-direction.

Furthermore the module can plot the electric field profile on an projection axis normal to the x,y or z-axis at a particular
plane in the sensor. Additional plots comprise the individual field vector components as well as the field magnitude and can
be enabled and controlled with the plotting parameters listed below.

## Parameters

* `model` : Type of the electric field model, either **linear**, **constant**, **parabolic**, **custom** or **mesh**.
* `depletion_depth` : Thickness of the depleted region. Used for all electric fields. When using the depletion depth for the
  **linear** model, no depletion voltage can be specified. Defaults to the full sensor thickness. The alias `field_depth` can
  be used for improved readability when using the model **mesh** (as the depletion depth in an externally generated field may
  be smaller than the field depth).

### Parameters for models `linear` and `constant`

* `bias_voltage` : Voltage over the whole sensor thickness. Used to calculate the electric field for the models **constant**
  and **linear**.
* `depletion_voltage` : Indicates the voltage at which the sensor is fully depleted. Used to calculate the electric field if
  the *model* parameter is equal to **linear**.
* `deplete_from_implants` : Indicates whether the sensor is depleted from the implants or the back side for the **linear**
  model. Defaults to true (depletion from the implant side).

### Parameters for model `parabolic`

* `minimum_field` : Value of the electric field in the minimum.
* `minimum_position` : Position of the electric field minimum along `z`, in local coordinates. Required to be located within
  the sensor volume.
* `maximum_field` : Value of the electric field at the electrode.

### Parameters for model `mesh`

* `file_name` : Location of file containing the meshed electric field data.
* `file_units` : Field units in which the file content is to be interpreted in, this defaults to `V/cm`.
* `field_mapping`: Description of the mapping of the field onto the sensor or pixel cell. Possible values are `SENSOR` for
  sensor-wide mapping, `PIXEL_FULL`, indicating that the map spans the full 2D plane and the field is centered around the
  pixel center, `PIXEL_HALF_TOP` or `PIXEL_HALF_BOTTOM` indicating that the field only contains only one half-axis along `y`,
  `HALF_LEFT` or `HALF_RIGHT` indicating that the field only contains only one half-axis along `x`, or `PIXEL_QUADRANT_I`,
  `PIXEL_QUADRANT_II`, `PIXEL_QUADRANT_III`, `PIXEL_QUADRANT_IV` stating that the field only covers the respective quadrant
  of the 2D pixel plane. In addition, the `PIXEL_FULL_INVERSE` mode allows loading full-plane field maps which are not
  centered around a pixel cell but the corner between pixels.
* `field_scale`:  Scaling factor of the electric field in x- and y-direction. By default, the scaling factors are set to
  `{1, 1}` and the field is used with its physical extent stated in the field data file. To scale the field in the
  z-direction, the parameter `field_depth` can be used.
* `field_offset`: Offset of the field in x- and y-direction. With this parameter and the mapping mode `SENSOR`, the field can
  be shifted e.g. by half a pixel pitch to accommodate for fields which have been simulated starting from the pixel center.
  The shift is applied in positive direction of the respective coordinate.

### Parameters for model `custom`

* `field_functions` : Single equation (for a field vector along the `z` axis only) or array of three equations (for the three
  components of a vector field). All three coordinates `x`, `y`, and `z` can be used, parameters need to be specified in
  consecutively numbered square brackets (`[0]`, `[1]`), starting with `[0]` for each of the equations.
* `field_parameters` : Array of values for the parameters of any equation defined in `field_equations`. Units can be used.
  The number of parameters given must match the sum of the number of free parameters from all defined equations.

## Plotting parameters

* `output_plots` : Determines if output plots should be generated. Disabled by default.
* `output_plots_steps` : Number of bins in both x- and y-direction in the 2D histogram used to plot the electric field in the
  detectors. Only used if `output_plots` is enabled.
* `output_plots_project` : Axis to project the 3D electric field on to create the 2D histogram. Either **x**, **y** or **z**.
  Only used if `output_plots` is enabled.
* `output_plots_projection_percentage` : Percentage on the projection axis to plot the electric field profile. For example if
  *output_plots_project* is **x** and this parameter is set to 0.5, the profile is plotted in the Y,Z-plane at the
  X-coordinate in the middle of the sensor. Default is 0.5.
* `output_plots_single_pixel`: Determines if the whole sensor has to be plotted or only a single pixel. Defaults to true
  (plotting a single pixel).

## Usage

An example to add a linear field with a bias voltage of -150 V and a full depletion voltage of -50 V to all the detectors,
apart from the detector named 'dut' where a specific meshed field from an INIT file is added, is given below

```ini
[ElectricFieldReader]
model = "linear"
bias_voltage = -150V
depletion_voltage = -50V

[ElectricFieldReader]
name = "dut"
model = "mesh"
# Should point to the example electric field in the repositories etc directory
file_name = "example_electric_field.init"
```

This example uses the parabolic field shape and defines a minimum field and position as well as the field at the electrode:

```ini
[ElectricFieldReader]
model = "parabolic"
# In local coordinates of the sensor, i.e. 100um below the center of the sensor along z:
minimum_position = -100um
minimum_field = 5200V/cm
maximum_field = 10000V/cm
```

An example for a custom field definition is given below. Here, a one-dimensional field is defined, which will be
automatically applied to the z-axis of the detector. Care should be take to use the proper variables in the formula, in this
case `z` for the respective coordinate.

```ini
[ElectricFieldReader]
model = "custom"
field_function = "[0]*z*z + [1]"
field_parameters = 12500V/mm/mm/mm, 5000V/cm
```

And finally, a three-dimensional custom field is defined with varying number of parameters per equation and using different
coordinates for the three dimensions of the field vector:

```ini
[ElectricFieldReader]
model = "custom"
# Parabolic in x and y, linear in z:
field_function = "[0]*x*y","[0]*x*y","[0]*z + [1]"
field_parameters = 12500V/mm/mm/mm, 12500V/mm/mm/mm, 6000V/cm/cm, 5000V/cm
```

[@pixelav]: https://cds.cern.ch/record/687440
