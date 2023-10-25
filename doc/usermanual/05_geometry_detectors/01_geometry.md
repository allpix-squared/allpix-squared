---
# SPDX-FileCopyrightText: 2022-2023 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Simulation Geometry"
weight: 1
---

Simulations are frequently performed for a set of different detectors (such as a beam telescope and a device under test). All
of these individual detectors together form what Allpix Squared defines as the geometry. Each detector has a set of
properties attached to it:

- A unique detector `name` to refer to the detector in the configuration.

- The `position` in the world frame. This is the position of the geometric center of the sensitive device (sensor) given in
  world coordinates as X, Y and Z s defined in [Section 5.1.1](#coordinate-systems) (note that any
  additional components like the chip and possible support layers are ignored when determining the geometric center).

- An `orientation_mode` that determines the way that the orientation is applied. This can be either `xyz`, `zyx` or `zxz`,
  **where `xyz` is used as default if the parameter is not specified**. Three angles are expected as input, which should
  always be provided in the order in which they are applied.

  - The `xyz` option uses extrinsic Euler angles to apply a rotation around the global X axis, followed by a rotation
    around the global Y axis and finally a rotation around the global Z axis.

  - The `zyx` option uses the **extrinsic Z-Y-X convention** for Euler angles, also known as Pitch-Roll-Yaw or 321
    convention. The rotation is represented by three angles describing first a rotation of an angle $`\phi`$ (yaw) about
    the Z axis, followed by a rotation of an angle $`\theta`$ (pitch) about the initial Y axis, followed by a third
    rotation of an angle $`\psi`$ (roll) about the initial X axis.

  - The `zxz` uses the **extrinsic Z-X-Z convention** for Euler angles instead. This option is also known as the 3-1-3 or
    the "x-convention" and the most widely used definition of Euler angles \[[@eulerangles]\].

  {{% alert title="Note" color="info" %}}
  It is highly recommended to always explicitly state the orientation mode instead of relying on the default configuration.
  {{% /alert %}}

- The `orientation` to specify the Euler angles in logical order (e.g. first X, then Y, then Z for the `xyz` method),
  interpreted using the method above (or with the `xyz` method if the `orientation_mode` is not specified). An example for
  three Euler angles would be
  ```ini
  orientation_mode = "zyx"
  orientation = 45deg 10deg 12deg
  ```
  which describes the rotation of 45° around the Z axis, followed by a 10° rotation around the initial Y axis, and finally]
  a rotation of 12° around the initial X axis.

  {{% alert title="Note" color="info" %}}
  All supported rotations are extrinsic active rotations, i.e. the vector itself is rotated, not the coordinate system. All
  angles in configuration files should be specified in the order they will be applied.
  {{% /alert %}}

- A `type` parameter describing the detector model, for example `timepix` or `mimosa26`. These models define the geometry
  and parameters of the detector. Multiple detectors can share the same model, several of which are shipped ready-to-use
  with the framework.

- An `alignment_precision_position` optional parameter to specify the alignment precision along the three global axes as
  described in [Section 3.3](../03_getting_started/03_detector_configuration.md).

- An optional parameter `alignment_precision_orientation` for the alignment precision in the three rotation angles as
  described in [Section 3.3](../03_getting_started/03_detector_configuration.md).

- An optional **electric or magnetic field** in the sensitive device. These fields can be added to a detector by special
  modules as demonstrated in [Section 3.7](../03_getting_started/07_extending_simulation_chain.md#electric-fields).

The detector configuration is provided in the detector configuration file as explained in
[Section 3.3](../03_getting_started/03_detector_configuration.md).

## Coordinate systems

Local coordinate systems for each detector and a global frame of reference for the full setup are defined. The global
coordinate system is chosen as a right-handed Cartesian system, and the rotations of individual devices are performed around
the geometric center of their sensor.

Local coordinate systems for the detectors are also right-handed Cartesian systems, with the x- and y-axes defining the
sensor plane. The origin of this coordinate system is the center of the lower left pixel in the grid, i.e. the pixel with
indices (0,0), whereas the z-axis pointing towards the readout connected to the sensor.
This simplifies calculations in the local coordinate system as all positions can either be stated in absolute
numbers or in fractions of the pixel pitch.

A sketch of the actual coordinate transformations performed, including the order of transformations, is given below. The
global coordinate system used for tracking of particles through detector setup is shown on the left side, while the local
coordinate system used to describe the individual sensors is located at the right. Both local and global coordinate systems are aligned by default.
Therefore, without any rotation, the sensor’s backplane (opposite to the plane where the readout is performed) is turned to the negative side of the z-axis.

![](./transformations.png)\
*Coordinate transformations from global to local and reverse. The first row shows the detector positions in the respective
coordinate systems in top view, the second row in side view.*

The global reference for time measurements is the beginning of the event, i.e. the start of the particle tracking through the
setup. The local time reference is the time of entry of the *first* primary particle of the event into the sensor. This means
that secondary particles created within the sensor inherit the local time reference from their parent particles in order to
have a uniform time reference in the sensor. It should be noted that Monte Carlo particles that start the local time frame on
different detectors do not necessarily have to belong to the same particle track.

## Changing and accessing the geometry

The geometry is needed at an early stage because it determines the number of detector module instantiations as explained
in [Section 4.4](../04_framework/04_modules.md#module-instantiation). The procedure of finding and loading the appropriate
detector models is explained in more detail in the [next section](./02_models.md).

The geometry is directly added from the detector configuration file described in
[Section 3.3](../03_getting_started/03_detector_configuration.md). The geometry manager parses this file on construction, and
the detector models are loaded and linked later during geometry closing as described above. It is also possible to add
additional models and detectors directly using `addModel` and `addDetector` (before the geometry is closed). Furthermore it
is possible to add additional points which should be part of the world geometry using `addPoint`. This can for example be
used to add the beam source to the world geometry.

The detectors and models can be accessed by name and type through the geometry manager using `getDetector` and `getModel`,
respectively. All detectors can be fetched at once using the `getDetectors` method. If the module is a detector-specific
module its related detector can be accessed through the `getDetector` method of the module base class instead (returns a null
pointer for unique modules) as follows:

```cpp
void run(Event* event) {
    // Returns the linked detector
    std::shared_ptr<Detector> detector = this->getDetector();
}
```

[@eulerangles]: https://mathworld.wolfram.com/EulerAngles.html
