## ElectricFieldReader
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>)  
**Status**: Functional  

#### Description
Adds an electric field to the detector from the standard supported sources. By default every detector has no electric field in the sensitive device.

The reader does work with two models of electric field to read:

* For *linear* electric fields it add a constant electric field in the z-direction towards the pixel implants.
* For electric fields in the *INIT* format it parses a file the INIT format used in the PixelAV software. An example of a electric field in this format can be found in *etc/example_electric_field.init* in the repository. An explanation of the format is available in the source code of this module.

Furthermore the module can produce a plot the electric field profile on an projection axis normal to the x,y or z-axis at a particular plane in the sensor.

#### Parameters
* `model` : Type of the electric field model, either **linear** or **init**.
* `voltage` : Voltage over the whole sensor thickness. Used to calculate the electric field if the *model* parameter is equal to **linear**.
* `file_name` : Location of file containing the electric field in the INIT format. Only used if the *model* parameter has the value **init**.
* `output_plots` : Determines if output plots should be generated (slows down simulation). Disabled by default.
* `output_plots_step` : Number of bins in both the X and Y direction in the 2D histogram used to plot the electric field in the detectors. Only used if `output_plots` is enabled.
* `output_plots_project` : Axis to project the 3D electric field on to create the 2D histogram. Either **x**, **y** or **z**. Only used if `output_plots` is enabled.
* `output_plots_projection_percentage` : Percentage on the projection axis to plot the electric field profile. For example if *output_plots_project* is **x** and this parameter is 0.5 the profile is plotted in the Y,Z-plane at the X-coordinate in the middle of the sensor.
* `output_plots_single_pixel`: Determines if the whole sensor has to be plotted or only a single pixel. Defaults to false (plotting the whole sensor).

#### Usage
An example to add a linear field of 50 volt bias to all the detectors, apart from the detector with name 'dut' where a specific INIT field is added, is given below

```ini
[ElectricFieldReader]
model = "linear"
voltage = 50V

[ElectricFieldReader]
name = "dut"
model = "init"
# Should point to the example electric field in the repositories etc directory
file_name = "example_electric_field.init"
```
