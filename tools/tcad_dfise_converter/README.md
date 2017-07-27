## TCAD DF-ISE mesh converter
This code takes as input the .grd and .dat files from TCAD simulations. The .grd file contains the vertex coordinates (3D or 2D) of each mesh node and the .dat file contains the module of each electric field vector component for each mesh node, grouped by model regions (such as silicon bulk or metal contacts). The regions are defined in the .grd file by grouping vertices into edges, faces and, consecutively, volumes or elements.

A new regular mesh is created by scanning the model volume in regular X Y and Z steps (not coinciding necessarily with original mesh nodes) and using a barycentric interpolation method to calculate the respective electric field vector on the new point. The interpolation uses the 4 closest, no-coplanar, neighbour vertex nodes that respective tetrahedron encloses the query point. For the neighbours search, the software uses the Octree implementation from the paper "Efficient Radius Neighbor Search in Three-dimensional Point Clouds" by J. Behley et al (see below).

The output .init file (with the same .grd and .dat prefixe) can be imported into allpix (see the User's Manual for details). The INIT file has a header followed by a list of columns organized as
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
To run the program the following should be executed from the installation folder:
```bash
bin/tcad_dfise_converter/dfise_converter -f <file_name_prefix> [<options>] [<arguments>]
```
The list with options can be accessed using the -h option.
Default values are assumed for the options not used. These are
-R <region> 		= "bulk"
-r <search radius>	= 1 um
-r <radius step>	= 0.5 um
-m <max radius>		= 10 um
-c <volume cut>		= std::numeric_limits<double>::min()
-x,y,z <mesh binning>	= 100 (option should be set using -x, -y and -z)

The output INIT file will be named with the same file_name_prefix as the .grd and .dat files.

INIT files are read by specifying a file_name containing an .INIT file. The mesh_plotter tool can be used from the installation folder as follows:
```bash
bin/tcad_dfise_converter/mesh_plotter -f <file_name> [<options>] [<arguments>]
```
The list with options and defaults is shown with the -h option. A default value of 100 is used for the binning, but this can be changed.
In a 3D mesh, the plane to be plotted must be identified by using the option -p with argument *xy*, *yz* or *zx*, defaulting to *yz*.
The data to be ploted can be selected with the -d option, the arguments are *ex*, *ey*, *ez* for the vector components or the default value *n* for the norm of the electric field.


#### Octree
[corresponding paper](http://jbehley.github.io/papers/behley2015icra.pdf):
J. Behley, V. Steinhage, A.B. Cremers. *Efficient Radius Neighbor Search in Three-dimensional Point Clouds*, Proc. of the IEEE International Conference on Robotics and Automation (ICRA), 2015.

Copyright 2015 Jens Behley, University of Bonn.
This project is free software made available under the MIT License. For details see the OCTREE LICENSE file.
