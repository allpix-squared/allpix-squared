## TCAD DF-ISE mesh converter
This code takes as input the .grd and .dat files from the TCAD simulations. The .grd file contains the vertex coordinates (3D or 2D) of each mesh node and the .dat file contains the module of each electric field vector component for each mesh node, grouped by model regions (such as silicon bulk or metal contacts). The regions are defined in the .grd file by grouping vertices into edges, faces and, consecutively, volumes or elements.
A new regular mesh is created by scanning the model volume in regular X Y and Z steps (not coinciding necessarily with original mesh nodes) and using a barycentric interpolation method to calculate the respective electric field vector on the new point. The interpolation uses the 4 closest, no-coplanar, neighbour vertex nodes that respective tetrahedron encloses the query point. 
For the neighbours search, the software uses the Octree implementation from the paper "Efficient Radius Neighbor Search in Three-dimensional Point Clouds" by J. Behley et al.
The output .init file (with the same .grd and .dat prefixe) is the INIT file to be used by allpix2. It contains the header of the INIT file folowed by a list with the columns organized as
```bash
node.x	node.y	node.z	e-field.x	e-field.y	e-field.z
```

#### Features
- TCAD DF-ISE file format reader.
- Fast radius neighbor search for three-dimensional point clouds.
- Barycentric interpolation between non-regular mesh points.
- Several cuts available on the interpolation algorithm variables. 
- Interpolated data visalisation tool.

#### Usage
Example .grd and .dat files can be found in the data folder with the example_data prefix.
To run the program you should do:
```bash
./tcad_dfise_converter -f <file_name_prefix> [<options>] [<arguments>]
```
The list with options can be accessed using the -h option.
Default values are assumed for the options not used. These are
-R <region> 		= "bulk"
-r <search radius>	= 1 um
-r <radius step>	= 0.5 um
-m <max radius>		= 10 um
-c <volume cut>		= std::numeric_limits<double>::min()
-x,y,z <mesh binning>	= 100 (NOTE: option should be -x, -y or -z)

The output INIT file will be named with the same file_name_prefix as the .grd and .dat files.

INIT files, with default name <file_name_prefix>_<x.bin>_<y.bin>_<z.bin>.init, can be read with the mesh_plot tool doing
```bash
./mesh_plot -f <file_name_prefix> [<options>] [<arguments>]
```
The list with options is also shown with the -h option.
For the file name, the default value is 100 for the binning.
In a 3D mesh, the plane to be ploted must be identified by using the option -p with agument "xy", "yz" or "zx".
The data to be ploted can be selected with the -d option.
In a .init file containing the components of the electric field, the arguments are "ex", "ey", "ez" for the vector components or "n" for the norm of the electric field.


#### Octree
[corresponding paper](http://jbehley.github.io/papers/behley2015icra.pdf):
J. Behley, V. Steinhage, A.B. Cremers. *Efficient Radius Neighbor Search in Three-dimensional Point Clouds*, Proc. of the IEEE International Conference on Robotics and Automation (ICRA), 2015.

Copyright 2015 Jens Behley, University of Bonn.
This project is free software made available under the MIT License. For details see the OCTREE LICENSE file.
