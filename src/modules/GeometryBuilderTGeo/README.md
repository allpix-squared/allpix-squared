## GeometryBuilderTGeo
**Maintainer**: Neal Gauvin (<neal.gauvin@unige.ch>)  
**Status**: OUTDATED (not supported)  

#### Description
Constructs an TGeo representation of the internal geometry. Creates all detector devices and also add optional appliances and an optional test structure. Code is based on Geant4 geometry construction in original AllPix. Only supports hybrid pixel detectors.

#### Parameters
* `world_material` : Material used to use to represent the world. There are two possible options, either **Vacuum** or **Air**.
* `world_size` : Size of the world (centered at the origin). Defaults to a box of 1x1x1 cubic meter.
* `build_appliances` : Determines if appliances are enabled.
* `appliances_type` : Type of the appliances to be constructed (see source code for options). Only used if *build_appliances* is enabled.
* `build_test_structures` : Determines if the test structure has to be build.
* `output_file` : Optional ROOT file to write the constructed geometry into
* `GDML_output_file` : Optional file to write geometry to in the GDML format. Can only be used if the used ROOT version has GDML support (will throw an error otherwise).

#### Usage
An example to construct a simple TGeo geometry without appliances and no test structure and a world of 5x5x5 cubic meters.

```ini
[GeometryBuilderTGeo]
world_material = "Air"
world_size = 5m 5m 5m
build_appliances = 0
build_test_structures = 0
```
