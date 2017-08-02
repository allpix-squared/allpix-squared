## GeometryBuilderGeant4
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>)  
**Status**: Functional  

#### Description
Constructs the Geant4 geometry from the internal geometry. First constructs the world geometry from the internal world size, with a certain margin, using a particular world material. Then continues to create all the detectors using their internal detector models.

All the available detector models are fully supported. This builder can create extra support layers of the following materials (note that these should be specified in lowercase):

* silicon
* plexiglass
* kapton
* copper
* epoxy
* carbonfiber
* g10
* solder

This module also allows to simulate misalignments of the setup. By setting one or both of the variables `alignment_precision` and `alignment_precision_rot`, detector misalignments are drawn from a normal distribution with a width given by the configured precision. This is performed for each detector and for each of the three Cartesian coorinates and Euler angles individually. The simulation of misalignments can be omitted by either not specifying the variables or setting all three components of the variable to zero.

#### Parameters
* `world_material` : Material of the world, should either be **air** or **vacuum**. Default to **air** if not specified.
* `world_margin_percentage` : Percentage of the world size to add extra compared to the internally calculated minimum world size. Defaults to 0.1, thus 10%.
* `world_minimum_margin` : Minimum absolute margin to add to all sides of the internally calculated minimum world size. Defaults to zero for all axis, thus not having any minimum margin.
* `alignment_precision` : Alignment precision of the detector setup in the three cartesian coordinates. If this variable is set, a random misalignment is applied to the detector position.
* `alignment_precision_rot` : Alignment precision of the detector setup in the three Euler angles. If this variable is set, a random misalignment is applied to the detector orientation.
* `GDML_output_file` : Optional file to write the geometry to in GDML format. Can only be used if this Geant4 version has GDML support (will throw an error otherwise). Otherwise also likely produces an error due to incomplete GDML implementation in Geant4.

#### Usage
To create a Geant4 geometry using vacuum as world material and with always exactly one meter added to the minimum world size on every side, the following configuration can be used.

```ini
[GeometryBuilderGeant4]
world_material = "vacuum"
world_margin_percentage = 0
world_minimum_margin = 1m 1m 1m
```
