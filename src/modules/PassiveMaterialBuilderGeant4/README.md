# PassiveMaterialBuilderGeant4
**Maintainer**: Koen van den Brandt (<kbrandt@nikhef.nl>)
**Status**: Functional

### Description
Constructs a passive material from the internal passive material description using Geant4.
The module requires a position, orientation, material and the sizes of type-specific parameters.
The new passive material is placed inside the world volume defined by the GeometryBuilderGeant4 module.

The supported passive material types are:

* box: a massive rectangular box. 
* cylinder: an cylinder with a definedinner and outer radius. Can be filled with another material.
* tube: a rectangular box with a defined inner and outer diameter. Can be filled with another material.
* sphere: a sphere with a defined innter and outer radius. Can be filled with another material.

### Dependencies

This module requires an installation Geant4. 
This module needs to be placed before GeometryBuilderGeant4 for passive materials to be added to the simulation world.

### General Parameters
* `type` : Passive material type. Options are shown above.
* `position` : Position of the passive material in the world. Defaults to 0mm 0mm 0mm.
* `material` : Material of which the passive material is made. Defaults to the world_material defined in GeometeryBuilderGeant4.
* `orientation` : Angles through which the passive material is rotated about the specified axis. Defaults to 0deg 0 deg 0 deg. 
* `orientation_mode` : Orientation mode which specifies the axis about which the passive material should be rotated. Defaults to xyz.

### Type-Specific Parameters
Box:
* `size` : Size, or x- and y- dimension, of the box. Defaults to 0mm 0mm
* `thickness` : Thickness, or z-dimension of the box. Defaults to 0mm.

Cylinder:
* `inner_radius` : Inner Radius of the  of the cylinder in the xy-plane. Defaults to 0mm.
* `outer_radius` : Outer Radius of the  of the cylinder in the xy-plane. Defaults to 0mm.
* `height` : Height, or z-dimension, of the cylinder. Defaults to 0mm.
* `starting_angle` : Angle which defines at what point on the circumference the cylinder should start in multiples of pi. Defaults to 0 pi.
* `arc_length` : Angle that defines how much of the circumference will be created in multiples of pi. 2 pi will mean the full circumference, pi will mean half the circumfrence etc. Defaults to 2 pi.
* `filling_material` : Material that will fill up the cylinder if inner_radius>0mm. Defaults to the world_material defined in GeometeryBuilderGeant4.

Tube:
* `inner_diameter` : The Inner Diameter of the  of the tube in the x and y-direction. Defaults to 0mm 0mm.
* `outer_diameter` : The Outer Diameter of the  of the tube in the x and y-direction. Defaults to 0mm 0mm.
* `length` : The Length, or z-dimension, of the tube. Defaults to 0mm.
* `filling_material` : Material that will fill up the tube if inner_diameter>0mm. Defaults to the world_material defined in GeometeryBuilderGeant4.

Sphere:
* `inner_radius` : Inner Radius of the  of the sphere. Defaults to 0mm.
* `outer_radius` : Outer Radius of the  of the sphere. Defaults to 0mm.
* `starting_angle_phi` :  Azimuthal Angle which defines at what point on the circumference the sphere should start in the xy-plane in multiples of pi. Defaults to 0 pi.
* `arc_length_phi` : Azimuthal Angle that defines how much of the circumference will be created in the xy-plane in multiples of pi. 2 pi will mean the full circumference, pi will mean half the circumfrence etc. Defaults to 2 pi.
* `starting_angle_theta` :  Polar Angle which defines at what point on the circumference the sphere should start as seen from the positive z-axis in multiples of pi. Defaults to 0 pi.
* `arc_length_theta` : Polar Angle that defines how much of the circumference will be created in multiples of pi. 2 pi will mean the full circumference, pi will mean half the circumfrence etc. Defaults to pi.
* `filling_material` : Material that will fill up the sphere if inner_diameter>0mm. Defaults to the world_material defined in GeometeryBuilderGeant4.

### Usage
To create a closed lead sphere with a wall thickness of 5mm and a radius of 20mm, filled with air. The sphere is not rotated and placed in the middle of the world.

```ini
[PassiveMaterialBuilderGeant4]
type = "sphere"
inner_radius = 15mm
outer_radius = 20mm
starting_angle_phi= 0
arc_length_phi = 2
starting_angle_theta = 0
arc_length_theta = 1
material = "lead"
filling_material = "air"
orientation = 0 0 0
position = 0mm 0mm 0mm

```

To create an open copper tube with a wall thickness of 1mm and a size of 8mm by 12mm, filled with air. The tube is not rotated and placed in the middle of the world.

```ini
[PassiveMaterialBuilderGeant4]
type = "tube"
inner_diamter = 6mm 10mm
outer_diamter = 8mm 12mm
material = "copper"
orientation = 0 0 0
position = 0mm 0mm 0mm

```
