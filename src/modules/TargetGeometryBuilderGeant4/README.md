# TargetGeometryBuilderGeant4
**Maintainer**: Koen van den Brandt (<kbrandt@nikhef.nl>)
**Status**: Functional

### Description
Constructs a target using Geant4 geometry from the internal geometry description. Can construct a target of given size, thickness and material on a given position. The new target is placed inside the world volume defined by the GeometryBuilderGeant4 module.


### Dependencies

This module requires an installation Geant4.

### Parameters
* `target_size` : size, or x- and y- dimension, of the target. Defaults to 0mm 0mm
* `target_thickness` : thickness, or z-dimension, of the target. Defaults to 0mm.
* `target_location` : Position of the target in the world geometry. Defaults to 0mm 0mm 0mm
* `target_material` : Material of the target. Defaults to the world-material which is defined in the GeometryBuilderGeant4 module.
* `GDML_output_file` : Optional file to write the geometry to in GDML format. Can only be used if this Geant4 version is built with GDML support enabled and will throw an error otherwise. This feature is to be considered experimental as the GDML implementation of Geant4 is incomplete.


### Usage
To create a lead cube with 1 mm sides in the center of the world you can use

[TargetGeometryBuilderGeant4]
target_size = 1mm 1mm
target_thickness = 1mm
target_location = 0mm 0mm 0mm
target_material = "lead"

```
