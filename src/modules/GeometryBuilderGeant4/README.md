# GeometryBuilderGeant4
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>)  
**Status**: Functional  

### Description
Constructs the Geant4 geometry from the internal geometry description.
First, the world frame with a configurable margin and material is constructed.
Then all passive materials and detectors using their internal detector models and passive material models are created and placed within the world frame or a specified mother volume (only for passive materials), which corresponds to another passive volume.
The descriptions of all detectors and passive volumes have to be specified within the geometry configuration.

All available detector models are fully supported.

For passive materials, the implemented models are "box", "cylinder" and "sphere".
The dimensions of the individual volumes are defined by the following parameters for the specific models and to be set within the corresponding section of the geometry configuration:

For each model, a set of specific size parameters need to be given, of which some are optional.\\
#### Box:
A rectangular box which can be massive or have an hole in the middle along the z-axis.
* The `size` of the box is an XYZ vector which defines the total size of the box.
* (Optional) The `inner_size` of the box is an XY vector which defines the size of the hole which will be removed from the original box volume. Defaults to 0mm 0mm.
* (Optional) The `thickness` of the box is a value which defines the thickness of the walls of a box with a hole in the middle. This has the has the same effect as the `inner_size`, and such they can't be used together.
 Defaults to 0mm.

#### Cylinder:
A cylindrical tube which can be massive or have an hole in the middle along the z-axis.
* The `outer_radius` of the cylinder is the total radius of the cylinder (in the XY-plane).
* The `length` of the cylinder is the total length of the cylinder (in the Z-direction).
* (Optional) The `inner_radius` of the cylinder is the radius of the inner cylinder. Defaults to 0mm.
* (Optional) The `starting_angle` of the cylinder is the angle at which circumference of the cylinder will start. 0 degrees refers to the point along the positive x-axis and the angle moves counter clockwise. Defaults to 0deg.
* (Optional) The `arc_length` of the cylinder is the arc-length of the circumference that will be drawn, starting from the given `starting_angle`. Defaults to 360deg which is the full circumference.
Note that the if the `arc_length` is set to 360 degrees, the \apsq framework will always draw the full circumference, regardless of the value of `starting_angle`.

#### Sphere:
A full or partly made sphere with an inner- and outer radius.
* The `outer_radius` of the sphere is the total radius of the sphere in all directions.
* (Optional) The `inner_radius` of the sphere is the radius of the inner sphere. Defaults to 0mm.
* (Optional) The `starting_angle_phi` of the sphere is the azimuthal angle at which circumference of the sphere will start in the XY-plane. 0 degrees refers to the point along the positive x-axis and the angle moves counter clockwise. Defaults to 0deg.
* (Optional) The `arc_length_phi` of the sphere is the arc-length of the circumference that will be drawn, starting from the given `starting_angle_phi` in the XY-plane. Defaults to 360deg which is the full circumference.
* (Optional) The `starting_angle_theta` of the sphere is the polar angle at which the `arc_length_theta` will start. 0 degrees refers to the point along the positive z-axis. Defaults to 0deg.
* (Optional) The `arc_length_theta` of the sphere is the arc-length of the polar angle which will be rotated around the z-axis to build the sphere, starting from the given `starting_angle_theta`. Defaults to 100deg which creates the full circle.\\
Note that `arc_length_phi` works the same as the `arc_length` from the cylinder, but the `arc_length_theta` works different.
The \apsq framework will only draw the full circle if `starting_angle_theta` = 0deg, and `arc_length_theta` = 180deg. 
In all other situations, the sphere will start at `starting_angle_theta` and continue the `arc_length_theta` untill `arc_length_theta` + `starting_angle_theta` = 180deg. After this it willl stop.
The necessary module errors and warnings have been included to make sure the user will know will and won't be build.
Note: If the VisualizationGeant4 module is used in conjunction with and `arc_length_theta` different from 180deg, the Visualization GUI will show an error "Inconsistency in bounding boxes for solid". The origin of this error is unknown but the error can be ignored.

This module can create support layers and passive volumes of the following materials:

* Air
* Aluminum
* Carbonfiber (a mixture of carbon and epoxy)
* Copper
* Epoxy
* G10 (PCB material)
* Kapton (using the `G4_KAPTON` definition)
* Lead
* Plexiglass (using the `G4_PLEXIGLASS` definition)
* Silicon
* Solder (a mixture of tin and lead)
* Tungsten
* Lithium
* Beryllium

### Dependencies

This module requires an installation of Geant4.

### Parameters
* `world_material` : Material of the world, should either be **air** or **vacuum**. Defaults to **air** if not specified.
* `world_margin_percentage` : Percentage of the world size to add to every dimension compared to the internally calculated minimum world size. Defaults to 0.1, thus 10%.
* `world_minimum_margin` : Minimum absolute margin to add to all sides of the internally calculated minimum world size. Defaults to zero for all axis, thus not requiring any minimum margin.

### Usage
To create a Geant4 geometry using vacuum as world material and with always exactly one meter added to the minimum world size in every dimension, the following configuration could be used:

```ini
[GeometryBuilderGeant4]
world_material = "vacuum"
world_margin_percentage = 0
world_minimum_margin = 1m 1m 1m
```
