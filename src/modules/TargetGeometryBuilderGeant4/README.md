# TargetGeometryBuilderGeant4
**Maintainer**: Koen van den Brandt (<kbrandt@nikhef.nl>)  
**Status**: Functional  

### Description
Constructs a target using Geant4 geometry from the internal geometry description. Can construct a target of given size, thickness and material on a given position. The new target is placed inside the world volume defined by the GeometryBuilderGeant4 module.


### Dependencies

This module requires an installation Geant4.

### Parameters

### Usage
To create a lead cube with 1 mm sides in the center of the world you can use

[TargetGeometryBuilderGeant4]
target_size = 1mm 1mm
target_thickness = 1mm
target_location = 0mm 0mm omm
target_material = "lead"

```
