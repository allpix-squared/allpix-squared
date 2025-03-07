---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Extending the Simulation Chain"
weight: 7
---

In the following, a few basic modules will be discussed which may be of use during a first simulation.

## Visualization

Displaying the geometry and the particle tracks helps both in checking and interpreting the results of a simulation.
Visualization is fully supported through Geant4, supporting all the options provided by Geant4 \[[@geant4vis]\]. Using the Qt
viewer with OpenGL driver is the recommended option as long as the installed version of Geant4 is built with Qt support
enabled.

To add the visualization, the `VisualizationGeant4` section should be added at the end of the configuration file. An example
configuration with some useful parameters is given below:

```ini
[VisualizationGeant4]
# Use the Qt gui
mode = "gui"

# Set opacity of the detector models (in percent)
opacity = 0.4
# Set viewing style (alternative is 'wireframe')
view_style = "surface"

# Color trajectories by charge of the particle
trajectories_color_mode = "charge"
trajectories_color_positive = "blue"
trajectories_color_neutral = "green"
trajectories_color_negative = "red"
```

If Qt is not available, a VRML viewer can be used as an alternative, however it is recommended to reinstall Geant4 with the
Qt viewer included as it offers the best visualization capabilities. The following steps are necessary in order to use a VRML
viewer:

* A VRML viewer should be installed on the operating system. Good options are FreeWRL or OpenVRML.

* Subsequently, two environmental parameters have to be exported to the shell environment to inform Geant4 about the
  configuration: `G4VRMLFILE_VIEWER` should point to the location of the viewer executable and should
  `G4VRMLFILE_MAX_FILE_NUM` typically be set to 1 to prevent too many files from being created.

* Finally, the configuration section of the visualization module should be altered as follows:

  ```ini
  [VisualizationGeant4]
  # Do not start the Qt gui
  mode = "none"
  # Use the VRML driver
  driver = "VRML2FILE"
    ```

More information about all possible configuration parameters can be found in the
[`VisualizationGeant4` documentation](../08_modules/visualizationgeant4.md).

## Electric Fields

By default, detectors do not have an electric field associated with them, and no bias voltage is applied. A field can be
added to each detector using the `ElectricFieldReader` module.

The section below calculates a linear electric field for every point in active sensor volume based on the depletion voltage
of the sensor and the applied bias voltage. The sensor is always depleted from the implant side. The direction of the
electric field depends on the sign of the bias voltage as described in the
[`ElectricFieldReader` documentation](../08_modules/electricfieldreader.md).

```ini
# Add an electric field
[ElectricFieldReader]
# Set the field type to `linear`
model = "linear"
# Applied bias voltage to calculate the electric field from
bias_voltage = -50V
# Depletion voltage at which the given sensor is fully depleted
depletion_voltage = -10V
```

Allpix Squared also provides the possibility to utilize a full electrostatic TCAD simulation for the description of the
electric field. In order to speed up the lookup of the electric field values at different positions in the sensor, the
adaptive TCAD mesh has to be interpolated and transformed into a regular grid with configurable feature size before use.
Allpix Squared comes with a converter tool which reads TCAD DF-ISE files from the sensor simulation, interpolates the field,
and writes this out in an appropriate format. A more detailed description of the tool can be found in
[Section 14.2](../14_additional/mesh_converter.md). An example electric field can be found in the repository \[[@ap2-repo]\]
at `etc/example_electric_field.init`. A detailed description of supported field geometries and their mapping onto the sensor
plane is provided in [Section 4.5](../04_framework/05_fieldmaps.md).

Electric fields can be attached to a specific detector using the
standard syntax for detector binding. A possible configuration would be:

```ini
[ElectricFieldReader]
# Bind the electric field to the detector named `dut`
name = "dut"
# Specify that the model is provided as meshed electric field map format, e.g. converted from TCAD
model = "mesh"
# Name of the file containing the electric field
file_name = "example_electric_field.init"
```

## Magnetic Fields

For simulating the detector response in the presence of a magnetic field with Allpix Squared, a constant, global magnetic
field can be defined. By default, it is turned off. A field can be added to the whole setup using the unique module
`MagneticFieldReader`, passing the field vector as parameter:

```ini
# Add a magnetic field
[MagneticFieldReader]
# Constant magnetic field (currently this is the default value)
model = "constant"
# Magnetic field vector
magnetic_field = 0mT 3.8T 0T
```

The global magnetic field is used by the interface to Geant4 and therefore exposes charged primary particles to the Lorentz
force, and as a property of each detector present, enabling a Lorentz drift of the charge carriers in the active sensors, if
supported by the used propagation modules. See the [Chapter 8](../08_modules/_index.md) for more information on the available
propagation modules.

Currently, only constant magnetic fields can be applied. For all parameters, refer to the
[`MagneticFieldReader` documentation](../08_modules/magneticfieldreader.md).


[@geant4vis]: https://geant4.web.cern.ch/geant4/UserDocumentation/UsersGuides/ForApplicationDeveloper/html/ch08.html
[@ap2-repo]: https://gitlab.cern.ch/allpix-squared/allpix-squared
