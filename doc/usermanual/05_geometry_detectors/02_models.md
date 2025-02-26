---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Detector Models"
weight: 2
---

Different types of detector models are available and distributed together with the framework: these models use the
configuration format introduced in [Section 4.3](../04_framework/03_configuration.md#file-format) and can be found in the `models`
directory of the repository. Every model extends from the `DetectorModel` base class, which defines the minimum required
parameters of a detector model within the framework. The coordinates place the detector in the global coordinate system, with
the reference point taken as the geometric center of the active matrix. This is defined by the number of pixels in the sensor
in both the x- and y-direction, and together with the pitch of the individual pixels the total size of the pixel matrix is
determined. Outside the active matrix, the sensor can feature excess material in all directions in the x-y-plane. A detector
of base class type does not feature a separate readout chip, thus only the thickness of an additional, inactive silicon layer
can be specified. Derived models allow for separate readout chips, optionally connected with bump bonds.

The base detector model can be extended to provide different sensor geometries, and new assembly types can be added for more
complex detector assembly setups. Currently, two assembly types are implemented, `MonolithicAssembly`, which describes a
monolithic detector with all electronics directly implemented in the same wafer as the sensor, and the `HybridAssembly`,
which in addition to the features described above also includes a separate readout chip with configurable size and bump bonds
between the sensor and readout chip.

## Detector model parameters

Models are defined in configuration files which are used to instantiate the actual model classes; these files contain various
types of parameters, some of which are required for all models while others are optional or only supported by certain model
types. For more details on how to add and use a new detector model, [Section 10.5](../10_development/05_new_detector_model.md)
should be consulted.

The set of base parameters supported by every model is provided below. These parameters should be given at the top of the
file before the start of any sub-sections.

- `geometry`:
  A required parameter describing the geometry of the model. At the moment either `pixel` or `radial_strip`. This value
  determines some of the supported parameters as discussed later.

- `type`:
  A required parameter describing the type of the detector assembly. At the moment either `monolithic` or `hybrid`. This
  value determines some of the supported parameters as discussed later.

- `number_of_pixels`:
  The number of pixels in the 2D pixel matrix. Determines the base size of the sensor together with the `pixel_size`
  parameter below.

- `pixel_size`:
  The pitch of a single pixel in the pixel matrix. Provided as 2D parameter in the x-y-plane. This parameter is required
  for all models.

- `sensor_material`:
  Semiconductor material of the sensor. This can be any of the sensor materials supported by Allpix Squared, currently
  `SILICON`, `GALLIUM_ARSENIDE`, `GERMANIUM`, `CADMIUM_TELLURIDE`, `CADMIUM_ZINC_TELLURIDE`, `DIAMOND` and
  `SILICON_CARBIDE`. Defaults to `SILICON` if not specified.

- `sensor_thickness`:
  Thickness of the active area of the detector model containing the individual pixels. This parameter is required for all
  models.

- `sensor_excess_<direction>`:
  With `<direction>` either `top`, `bottom`, `right` or `left`, where the `top`, `bottom`, `right` and `left` direction are
  the positive y-axis, the negative y-axis, the positive x-axis and the negative x-axis, respectively. Specifies the extra
  material added to the sensor outside the active pixel matrix in the given direction.

- `sensor_excess`:
  Fallback for the excess width of the sensor in all four directions (top, bottom, right and left). Used if the specialized
  parameters described below are not given. Defaults to zero, thus having a sensor size equal to the number of pixels times
  the pixel pitch.

- `chip_thickness`:
  Thickness of the readout chip, placed next to the sensor.

The base parameters described above are the only set of parameters supported by the **monolithic** assembly. For this
assembly, the `chip_thickness` parameter represents the first few micrometers of sensor material which contain the chip
circuitry and are shielded from the bias voltage and thus do not contribute to the signal formation.

The **hybrid** assembly adds bump bonds between the chip and sensor while automatically making sure the chip and support
layers are shifted appropriately. Furthermore, it allows the user to specify the chip dimensions independently from the
sensor size, as the readout chip is treated as a separate entity. The additional parameters for the **hybrid** assembly are
the following:

- `chip_excess_<direction>`:
  With direction either `top`, `bottom`, `right` or `left`. The chip excess in the specific direction, similar to the
  `sensor_excess_<direction>` parameter described above.

- `chip_excess`:
  Fallback for the excess width of the chip, defaults to zero and thus to a chip size equal to the dimensions of the pixel
  matrix. See the `sensor_excess` parameter above.

- `bump_height`:
  Height of the bump bonds (the separation distance between the chip and the sensor).

- `bump_sphere_radius`:
  The individual bump bonds are simulated as union solids of a sphere and a cylinder. This parameter sets the radius of the
  sphere to use.

- `bump_cylinder_radius`:
  The radius of the cylinder part of the bump. The height of the cylinder is determined by `bump_height` the parameter.

- `bump_offset`:
  A 2D offset of the grid of bumps. The individual bumps are by default positioned at the center of each single pixel in
  the grid.


## Specializing detector models

A detector model contains default values for all parameters. Some parameters like the sensor thickness can however vary
between different detectors of the same model. To allow for easy adjustment of these parameters, models can be specialized in
the detector configuration file introduced in [Section 3.3](../03_getting_started/03_detector_configuration.md). All model
parameters, except the type parameter and the support layers, can be changed by adding a parameter with the same key
and the updated value to the detector configuration. The framework will then automatically create a copy of this model with
the requested change.

{{% alert title="Note" color="info" %}}
Before re-implementing models, it should be checked if the desired change can be achieved using the detector model
specialization. For most cases this provides a quick and flexible way to adapt detectors to different needs and setups (for
example, detectors with different sensor thicknesses).
{{% /alert %}}

## Search order for models

To support different detector models and storage locations, the framework searches different paths for model files in the
following order:

1. If defined, the paths provided in the global `model_paths` parameter are searched first. Files are read and parsed
   directly. If the path is a directory, all files in the directory are added (without recursing into subdirectories).

2. The location where the models are installed to (refer to the description of the `MODEL_DIRECTORY` variable in
   [Section 2.5](../02_installation/05_cmake_configuration.md)).

3. The standard data paths on the system as given by the environmental variable `XDG_ DATA_DIRS` with `Allpix/models`
   appended. The variable defaults to `/usr/local/share/` (thus effectively `/usr/local/share/Allpix/models`) followed by
   `/usr/share/` (effectively `/usr/share/Allpix/models`).

4. The path of the main configuration file.


## Implants

Multiple implants per pixel cell can be simulated in Allpix Squared. Here, implants are any volume in the sensor in which
charge carriers do not propagated, such as collection diodes, ohmic volumes or columns, as well as trenches filled with
different materials e.g. for alpha conversion.

When charge carriers reach an implant, their propagation is stopped. Depending on the type of implant, they might be either
discarded by the transfer module for back-side implants, or taken into account when forming the front-end electronics input
signal for front-side implants.

Each implant should be defined in its own section headed with the name `[implant]`. By default, no implants are added.
Implants allow for the following parameters:

- `type`:
  Type of the implant. This parameter can be set to either `frontside` for an implant from the sensor front side, collecting
  charge carriers, or to `backside` for an implant connected to the ohmic contact at the sensor back side.

- `shape`:
  Shape of the implant, supported shapes are `rectangle` and `ellipse`. Defaults to `rectangle`.

- `size`:
  The size of the implant as 3D vector with size in $`x`$ and $`y`$ as well as the implant depth. Depending on the implant shape,
  the $`x`$ and $`y`$ values are either interpreted as the side lengths of the rectangle or the major and minor axes of the
  ellipse.

- `orientation`:
  Rotation of the implant around its $`z`$ axis. Defaults to 0 degrees, i.e. the implant axes are aligned with the local
  coordinate system of the sensor.

- `offset`:
  2D values in the x-y plane, defining the offset of the implant from the center of the pixel cell. This parameter is
  optional and defaults to $`0, 0`$, i.e. a position at the pixel center be default.


## Support Layers

In addition to the active layer, multiple layers of support material can be added to the detector description. It is possible
to place support layers at arbitrary positions relative to the sensor, while the default position is behind the readout chip
or inactive sensor layer. The defined support materials will always be positioned relative to the corresponding detector. The
support material can be chosen either from a set of predefined materials, including PCB and Kapton, or any material available
via the Geant4 material database.

Every support layer should be defined in its own section headed with the name `[support]`. By default, no support layers are
added. Support layers allow for the following parameters.

- `size`:
  Size of the support in 2D (the thickness is given separately below). This parameter is required for all support layers.

- `thickness`:
  Thickness of the support layers. This parameter is required for all support layers.

- `location`:
  Location of the support layer. Either `sensor` to attach it to the sensor (opposite to the readout chip/inactive sensor
  layer), `chip` to add the support layer behind the chip/inactive layer or `absolute` to specify the offset in the
  z-direction manually. Defaults to `chip` if not specified. If the parameter is equal to `sensor` or `chip`, the support
  layers are stacked in the respective direction when multiple layers of support are specified.

- `offset`:
  If the `location` parameter is equal to `sensor` or `chip`, an optional 2D offset can be specified using this parameter,
  the offset in the z-direction is then automatically determined. These support layers are by default centered around the
  middle of the pixel matrix (the rotation center of the model). If the `location` is set to `absolute`, the offset is a
  required parameter and should be provided as a 3D vector with respect to the center of the model (thus the center of the
  active sensor). Care should be taken to ensure that these support layers and the rest of the model do not overlap.

- `hole_size`:
  Adds an optional cut-out hole to the support with the 2D size provided. The hole always cuts through the full support
  thickness. No hole will be added if this parameter is not present.

- `hole_type`:
  Type of hole to be punched into the support layer. Currently supported are `rectangle` and `cylinder`. Defaults to
  `rectangle`.

- `hole_offset`:
  If present, the hole is by default placed at the center of the support layer. A 2D offset with respect to its default
  position can be specified using this parameter.

- `material`:
  Material of the support. Allpix Squared does not provide a set of materials to choose from; it is up to the modules using
  this parameter to implement the materials such that they can use it. [Chapter 8](../08_modules/_index.md) provides
  details about the materials supported by the geometry builder modules (for example in the
  [`GeometryBuilderGeant4 module documentation`](../08_modules/geometrybuildergeant4.md)).


## Accessing specific detector models within the framework

Some modules are written to act on only a particular type of detector model. In order to ensure that a specific detector
model has been used, the model should be downcast: the downcast returns a null pointer if the class is not of the appropriate
type. An example for fetching a `HybridPixelDetectorModel` would thus be:

```cpp
// "detector" is a pointer to a Detector object
auto model = detector->getModel();
auto hybrid_model = std::dynamic_pointer_cast<HybridPixelDetectorModel>(model);
if(hybrid_model != nullptr) {
    // The model of this Detector is a HybridPixelDetectorModel
}
```
