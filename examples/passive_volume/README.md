# EUDET Telescope Simulation Example

This example showcases the definition of passive volumes in the geometry file.

The file `example_detector_passive.conf` contains a detector of the type `test`, as well as several passive objects, identified via the key-paramater pair `role = "passive"`. The example shows the three basic objects implemented, while for the volume "sphere1" the "box1" is defined as `mother_volume`. This implies, that the sphere is integrated into the box and that the given position (and orientation, if applicable) are interpreted relative to the position and orientation of the box.

The `VisualizationGeant4` should visualize these objects.

All further modules contain standard parameters.