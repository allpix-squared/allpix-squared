# TCAD DF-ISE mesh converter

This code takes as input the .grd and .dat files from the TCAD simulations. The .grd file contains the vertex coordinates (3D or 2D) of each mesh node and the .dat file contains the module of each electric field vector component for each mesh node, grouped by model regions (such as silicon bulk or metal contacts). The regions are defined in the .grd file by grouping vertices into edges, faces and, consecutively, volumes or elements.
A new regular mesh is created by scanning the model volume in regular X Y and Z steps (not coinciding necessarily with original mesh nodes) and using a barycentric interpolation method to calculate the respective electric field vector on the new point. The interpolation uses the 4 closest, no-coplanar, neighbour vertex nodes that respective tetrahedron encloses the query point. 
For the neighbours search, the software uses the Octree implementation from the paper "Efficient Radius Neighbor Search in Three-dimensional Point Clouds" by J. Behley et al.
The output .txt file (with the same .grd and .dat prefixes) is the INIT file to be used by allpix2. It contains the header of the INIT file folowed by a list with the columns organized as: node.x node.y node.z e-field.x e-field.y e-field.z

## Features
- TCAD DF-ISE file format reader.
- Fast radius neighbor search for three-dimensional point clouds.
- Barycentric interpolation between non-regular mesh points

## Usage
To run the program you should do
```bash
./tcad_dfise_converter <data_path> [x_pitch y_pitch z_pitch]
```
If the pitch is not defined, default value = 100 will be used

To download some example data do:
Vertices coordinates:
```bash
wget https://gitlab.cern.ch/mvicente/octrees/blob/master/data/test_structure_0V.grd
```
Vertices data:
```bash
wget https://gitlab.cern.ch/mvicente/octrees/blob/master/data/test_structure_0V.dat
```

## Octree
[corresponding paper](http://jbehley.github.io/papers/behley2015icra.pdf):
J. Behley, V. Steinhage, A.B. Cremers. *Efficient Radius Neighbor Search in Three-dimensional Point Clouds*, Proc. of the IEEE International Conference on Robotics and Automation (ICRA), 2015.

### License
Copyright 2015 Jens Behley, University of Bonn.
This project is free software made available under the MIT License. For details see the OCTREE LICENSE file.
