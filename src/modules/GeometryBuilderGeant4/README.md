---
# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "GeometryBuilderGeant4"
description: "Constructs the Geant4 geometry"
module_status: "Functional"
module_maintainers: ["Paul Schuetze (<paul.schuetze@desy.de>)"]
---

## Description
Constructs the Geant4 geometry from the internal geometry description.
First, the world frame with a configurable margin and material is constructed.
Then all passive materials and detectors using their internal detector models and passive material models are created and placed within the world frame or a specified mother volume (only for passive materials), which corresponds to another passive volume.
The descriptions of all detectors and passive volumes have to be specified within the geometry configuration.

All available detector models are fully supported.

### Passive Volumes

For passive materials, the implemented models are "box", "cylinder", "sphere" as well as "gdml".
The dimensions of the individual volumes are defined by the following parameters for the specific models and to be set within the corresponding section of the geometry configuration:

For each model, a set of specific size parameters need to be given, of which some are optional.

#### Box:
A rectangular box which can be massive or have an hole in the middle along the z-axis.

* The `size` of the box is an XYZ vector which defines the total size of the box.
* (Optional) The `inner_size` of the box is an XYZ vector which defines the size of the volume that will be removed at the center of the original box volume. Defaults to `0mm 0mm 0mm` (no volume removed).
* (Optional) The `thickness` of the box is a value which defines the thickness of the walls of a box. This has a similar effect as the parameter `inner_size`, and such they can't be used together. Defaults to 0mm.

#### Cylinder:
A cylindrical tube which can be massive or have an hole in the middle along the z-axis.

* The `outer_radius` of the cylinder is the total radius of the cylinder (in the XY-plane).
* The `length` of the cylinder is the total length of the cylinder (in the Z-direction).
* (Optional) The `inner_radius` of the cylinder is the radius of the inner cylinder. Defaults to 0mm.
* (Optional) The `starting_angle` of the cylinder is the angle at which circumference of the cylinder will start. 0 degrees refers to the point along the positive x-axis and the angle moves counter clockwise. Defaults to 0deg.
* (Optional) The `arc_length` of the cylinder is the arc-length of the circumference that will be drawn, starting from the given `starting_angle`. Defaults to 360deg which is the full circumference.

Note that the if the `arc_length` is set to 360 degrees, the Allpix Squared framework will always draw the full circumference, regardless of the value of `starting_angle`.

#### Sphere:
A full or partly made sphere with an inner- and outer radius.

* The `outer_radius` of the sphere is the total radius of the sphere in all directions.
* (Optional) The `inner_radius` of the sphere is the radius of the inner sphere. Defaults to 0mm.
* (Optional) The `starting_angle_phi` of the sphere is the azimuthal angle at which circumference of the sphere will start in the XY-plane. 0 degrees refers to the point along the positive x-axis and the angle moves counter clockwise. Defaults to 0deg.
* (Optional) The `arc_length_phi` of the sphere is the arc-length of the circumference that will be drawn, starting from the given `starting_angle_phi` in the XY-plane. Defaults to 360deg which is the full circumference.
* (Optional) The `starting_angle_theta` of the sphere is the polar angle at which the `arc_length_theta` will start. 0 degrees refers to the point along the positive z-axis. Defaults to 0deg.
* (Optional) The `arc_length_theta` of the sphere is the arc-length of the polar angle which will be rotated around the z-axis to build the sphere, starting from the given `starting_angle_theta`. Defaults to 100deg which creates the full circle.

Note that `arc_length_phi` works the same as the `arc_length` from the cylinder, but the `arc_length_theta` works different.
The Allpix Squared framework will only draw the full circle if `starting_angle_theta` = 0deg, and `arc_length_theta` = 180deg.
In all other situations, the sphere will start at `starting_angle_theta` and continue the `arc_length_theta` until `arc_length_theta` + `starting_angle_theta` = 180deg. After this it will stop.
The necessary module errors and warnings have been included to make sure the user will know will and won't be build.

Note: If the VisualizationGeant4 module is used in conjunction with and `arc_length_theta` different from 180deg, the Visualization GUI will show an error "Inconsistency in bounding boxes for solid". The origin of this error is unknown but the error can be ignored.

#### Cone:
A cone or partly made cone with an inner and outer radius at the (- half-length , + half-length)
 outer_radius_mDz : at - half-length the outer radius
                outer_radius_pDz : at + half-length the outer radius
                inner_radius_mDz : at - half-length the inner radius (must be smaller than outer_radius_mDz)
                inner_radius_pDz : at + half-length the inner radius (must be smaller than outer_radius_mDz)
                starting_angle   : start-angle ( default 0)
                arc_length       : length-of the arc (360 deg default)
* The `outer_radius_pDz` of the cone is the radius of the cone at the + half-lenght position
* The `outer_radius_mDz` of the cone is the radius of the cone at the - half-lenght position
* The `inner_radius_pDz` of the cone is the radius of the cone at the + half-lenght position
* The `inner_radius_mDz` of the cone is the radius of the cone at the - half-lenght position
* The `inner_radius_mDz` of the cone is the radius of the cone at the - half-lenght position
* The `length`  of the cone is the total length of the cone
* (Optional) The `starting_angle` of the cone is the azimuthal angle at which circumference of the cone will start in the XY-plane. 0 degrees refers to the point along the positive x-axis and the angle moves counter clockwise. Defaults to 0deg.
* (Optional) The `arc_length` of the cone is the arc-length of the cone that will be drawn
Note that `arc_length` works the same as the `arc_length` from the cylinder

#### GDML:
This model allows to load arbitrary GDML files \[[@gdml]\] as passive materials. All volumes from the GDML file which are contained within the world volume are processed and added to the geometry of the simulation.
The only parameter specific to this model is `file_name` which should provide the path to the GDML file to be read.

This functionality requires Geant4 to be built with GDML support enabled. This can be enabled via CMake when compiling Geant4 using

```
cmake -DDGEANT4_USE_GDML=ON ..
```

### Visualization Options

For each of the above mentioned models, a color and opacity can be added to the passive material.

* The `color` of the passive material is given in an R G B vector, where each color value is between 0 and 1. Defaults to `color = 0 0 1` (blue).
* The `opacity` of the passive material is given as a number between 0 and 1, where 0 is completely transparent, and 1 is completely opaque.


### Materials

The following materials are pre-defined and can directly be used for the world volume, detector support layers as well as passive volumes:
This module can create support layers and passive volumes of the following materials:

* Materials listed by Geant4:
    * air
    * aluminum
    * beryllium
    * copper
    * kapton
    * lead
    * lithium
    * plexiglass
    * silicon
    * germanium
    * tungsten
    * gallium arsenide
    * cadmium telluride
    * nickel
    * gold
    * titanium
* Composite or custom materials:
    * carbon fiber
    * epoxy
    * fused silica
    * PCB G-10
    * paper (cellulose)
    * solder
    * polystyrene
    * ppo foam
    * cadmium zinc telluride
    * diamond
    * silicon carbide
    * titanium grade 5
    * vacuum

Furthermore, this module can automatically load any material defined in the Geant4 material database \[[@g4materials]\]. This comprises both simple materials and pre-defined NIST compounds.
It should be noted that when loading a material from the Geant4 material database, the name comparison is case sensitive. Names can be provided with or without `G4_` prefix.

## Dependencies

This module requires an installation of Geant4.

## Parameters
* `world_material` : Material of the world, should either be **air** or **vacuum**. Defaults to **air** if not specified.
* `world_margin_percentage` : Percentage of the world size to add to every dimension compared to the internally calculated minimum world size. Defaults to 0.1, thus 10%.
* `world_minimum_margin` : Minimum absolute margin to add to all sides of the internally calculated minimum world size. Defaults to zero for all axis, thus not requiring any minimum margin.
* `log_level_g4cerr`: Target logging level for Geant4 messages from the G4cerr (error) stream. Defaults to `WARNING`.
* `log_level_g4cout`: Target logging level for Geant4 messages from the G4cout stream. Defaults to `TRACE`.

## Usage
To create a Geant4 geometry using vacuum as world material and with always exactly one meter added to the minimum world size in every dimension, the following configuration could be used:

```ini
[GeometryBuilderGeant4]
world_material = "vacuum"
world_margin_percentage = 0
world_minimum_margin = 1m 1m 1m
```

[@g4materials]: https://geant4-userdoc.web.cern.ch/UsersGuides/ForApplicationDeveloper/html/Appendix/materialNames.html
[@gdml]: https://gdml.web.cern.ch/GDML/
