# PassiveMaterialBuilderGeant4
**Maintainer**: Koen van den Brandt (<kbrandt@nikhef.nl>)
**Status**: Functional

### Description
Constructs a passive material from the internal passive material description using Geant4.
Creates all passive materials using the parameters specified for each passive material model.
The new passive material is placed inside the world volume defined by the GeometryBuilderGeant4 module.

The supported passive material models are:

* box: a massive rectangular box. Can contain an opening oriented along the z-direction.
* cylinder: an cylinder with a defined length, inner- and outer radius. The opening (if aplicable) is oriented along the z-direction.
* sphere: a sphere with a defined innter and outer radius.

### Dependencies

This module requires an installation Geant4. 
This module needs to be placed before GeometryBuilderGeant4 for passive materials to be added to the simulation world. Otherwise an error will occur.

### Parameters

### Usage
To add the passive materials defined in the passive_material_file:

```ini
[PassiveMaterialBuilderGeant4]
```
