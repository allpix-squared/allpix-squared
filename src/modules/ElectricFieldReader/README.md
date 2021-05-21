# ElectricFieldReader
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Functional

### Description
Adds an electric field to the detector from one of the supported sources. By default, detectors do not have an electric field applied.

The reader provides the following models for electric fields:

* For *constant* electric fields it add a constant electric field in the z-direction towards the pixel implants. This is not very physical but might aid in developing and testing new charge propagation algorithms.
* For *linear* electric fields, the field has a constant slope determined by the bias voltage and the depletion voltage. The sensor is depleted either from the implant or the back side, the direction of the electric field depends on the sign of the bias voltage (with negative bias voltage the electric field vector points towards the backplane and vice versa). If the sensor is depleted from the implant side, the electric field is calculated using the formula $`E(z) = \frac{U_{bias} - U_{depl}}{d} + 2 \frac{U_{depl}}{d}\left( 1- \frac{z}{d} \right)`$, where d is the thickness of the sensor, and $`U_{depl}`$, $`U_{bias}`$ are the depletion and bias voltages, respectively. In case of a depletion from the back side, the electric field is calculated as $`E(z) = \frac{U_{bias} - U_{depl}}{d} + 2 \frac{U_{depl}}{d}\left( \frac{z}{d} \right)`$.
* For *parabolic* electric fields, a parabola is defined in order to emulate a double-peaked field such as the electric fields observed in sensors after irratiation. The parabola is calculated from the position $`z_{min}`$ and value $`E_{min}`$ of the minimum field in the sensor and the maximum field value at the electrode, $`E_{max}`$. The parameters of parabolic equation $`E(z) = az^2 + bz + c`$ then resolve to:
    ```latex
    a = \frac{E_{max} - E_{min}}{z_{min}^2 + (d/2)^2 - dz_{min}}
    b = -2az_{min}
    c = E_{max} - a ((d/2)^2 - dz_{min})
    ```
    where $`d`$ is the sensor thickness and $`z`$ the position along the `z`-axis in local coordinates.
* For electric fields in the *INIT* or *APF* formats it parses a file containing an electric field map in the APF format or the legacy INIT format also used by the PixelAV software [@pixelav]. An example of a electric field in this format can be found in *etc/example_electric_field.init* in the repository. An explanation of the format is available in the source code of this module, a converter tool for electric fields from adaptive TCAD meshes is provided with the framework. Fields of different sizes can be used and mapped onto the pixel matrix using the `field_scale` parameter. By default, the module assumes the field represents a single pixel unit cell. If the field size and pixel pitch do not match, a warning is printed and the field is scaled to the pixel pitch.

The `depletion_depth` parameter can be used to control the thickness of the depleted region inside the sensor.
This can be useful for devices such as HV-CMOS sensors, where the typical depletion depth but not necessarily the full depletion voltage are know.
It should be noted that `depletion_voltage` and `depletion_depth` are mutually exclusive parameters and only one at a time can be specified.

Furthermore the module can produce a plot the electric field profile on an projection axis normal to the x,y or z-axis at a particular plane in the sensor.

### Parameters
* `model` : Type of the electric field model, either **linear**, **constant**, **parabolic** or **mesh**.
* `bias_voltage` : Voltage over the whole sensor thickness. Used to calculate the electric field if the *model* parameter is equal to **constant** or **linear**.
* `depletion_voltage` : Indicates the voltage at which the sensor is fully depleted. Used to calculate the electric field if the *model* parameter is equal to **linear**.
* `depletion_depth` : Thickness of the depleted region. Used for all electric fields. When using the depletion depth for the **linear** model, no depletion voltage can be specified.
* `deplete_from_implants` : Indicates whether the sensor is depleted from the implants or the back side for the **linear** model. Defaults to true (depletion from the implant side).
* `minimum_field` : Value of the electric field in the minimum. Only used for the **parabolic** model.
* `minimum_position` : Position of the electric field minimum along `z`, in local coordinates. Only used for the **parabolic** model.
* `maximum_field` : Value of the electric field at the electrode. Only used for the **parabolic** model.
* `file_name` : Location of file containing the meshed electric field data. Only used if the *model* parameter has the value **mesh**.
* `field_scale` : Scale of the electric field in x- and y-direction. This parameter allows to use electric fields for fractions or multiple pixels. For example, an electric field calculated for a quarter pixel cell can be used by setting this parameter to `0.5 0.5` (half pitch in both directions) while a field calculated for four pixel cells in y and a single cell in x could be mapped to the pixel grid using `1 4`. Defaults to `1.0 1.0`. Only used if the *model* parameter has the value **mesh**.
* `field_offset`: Offset of the field from the pixel edge in x- and y-direction. By default, the framework assumes that the provided electric field starts at the edge of the pixel, i.e. with an offset of `0.0`. With this parameter, the field can be shifted e.g. by half a pixel pitch to accommodate for fields which have been simulated starting from the pixel center. In this case, a parameter of `0.5 0.5` should be used. The shift is applied in positive direction of the respective coordinate. Only used if the *model* parameter has the value **mesh**.
* `output_plots` : Determines if output plots should be generated. Disabled by default.
* `output_plots_steps` : Number of bins in both x- and y-direction in the 2D histogram used to plot the electric field in the detectors. Only used if `output_plots` is enabled.
* `output_plots_project` : Axis to project the 3D electric field on to create the 2D histogram. Either **x**, **y** or **z**. Only used if `output_plots` is enabled.
* `output_plots_projection_percentage` : Percentage on the projection axis to plot the electric field profile. For example if *output_plots_project* is **x** and this parameter is set to 0.5, the profile is plotted in the Y,Z-plane at the X-coordinate in the middle of the sensor. Default is 0.5.
* `output_plots_single_pixel`: Determines if the whole sensor has to be plotted or only a single pixel. Defaults to true (plotting a single pixel).

### Usage
An example to add a linear field with a bias voltage of -150 V and a full depletion voltage of -50 V to all the detectors, apart from the detector named 'dut' where a specific meshed field from an INIT file is added, is given below

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

[@pixelav]: https://cds.cern.ch/record/687440
