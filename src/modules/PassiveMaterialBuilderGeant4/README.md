# PassiveMaterialBuilderGeant4
**Maintainer**: Koen van den Brandt (<kbrandt@nikhef.nl>)
**Status**: Functional

### Description
Constructs a passive material using Geant4 geometry from the internal geometry description. Can construct a passive material of given size, thickness and material on a given position. The new passive material is placed inside the world volume defined by the GeometryBuilderGeant4 module.


### Dependencies

This module requires an installation Geant4.

### Parameters
* `passive_material_size` : size, or x- and y- dimension, of the passive material. Defaults to 0mm 0mm
* `passive_material_thickness` : thickness, or z-dimension, of the passive material. Defaults to 0mm.
* `passive_material_location` : Position of the passive material in the world geometry. Defaults to 0mm 0mm 0mm
* `passive_material_material` : Material of the passive material. Defaults to the world-material which is defined in the GeometryBuilderGeant4 module.

### Usage
To create a lead cube with 1 mm sides in the center of the world you can use

[PassiveMaterialBuilderGeant4]
passive_material_size = 1mm 1mm
passive_material_thickness = 1mm
passive_material_location = 0mm 0mm 0mm
passive_material_material = "lead"

```
