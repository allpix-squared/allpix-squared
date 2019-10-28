# PassiveMaterialBuilderGeant4
**Maintainer**: Koen van den Brandt (<kbrandt@nikhef.nl>)
**Status**: Functional

### Description
Constructs a passive material from the internal passive material description using Geant4.
The module requires a position, orientation, material and the sizes of type-specific parameters.
The new passive material is placed inside the world volume defined by the GeometryBuilderGeant4 module.

The supported passive material types are:

* box: a massive rectangular box. Can be filled with another material.
* cylinder: an cylinder with a defined inner and outer radius. Can be filled with another material.
* tube: a rectangular box with a defined inner and outer diameter. Can be filled with another material.
* sphere: a sphere with a defined innter and outer radius. Can be filled with another material.

### Dependencies

This module requires an installation Geant4. 
This module needs to be placed before GeometryBuilderGeant4 for passive materials to be added to the simulation world.

### Parameters

### Usage
To add the passive materials defined in the passive_material_file:

```ini
[PassiveMaterialBuilderGeant4]
```
