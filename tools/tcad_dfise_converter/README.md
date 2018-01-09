## TCAD DF-ISE mesh converter
This code takes the .grd and .dat files of the DF-ISE format from TCAD simulations as input. The .grd file contains the vertex coordinates (3D or 2D) of each mesh node and the .dat file contains the value of each electric field vector component for each mesh node, grouped by model regions (such as silicon bulk or metal contacts). The regions are defined in the .grd file by grouping vertices into edges, faces and, consecutively, volumes or elements.

A new regular mesh is created by scanning the model volume in regular X Y and Z steps (not necessarily coinciding with original mesh nodes) and using a barycentric interpolation method to calculate the respective electric field vector on the new point. The interpolation uses the four closest, no-coplanar, neighbor vertex nodes such, that the respective tetrahedron encloses the query point. For the neighbors search, the software uses the Octree implementation [@octree].

The output .init file (with the same name as the .grd and .dat files) can be imported into Allpix Squared. The INIT file has a header followed by a list of columns organized as
```bash
node.x	node.y	node.z	observable.x	observable.y	observable.z
```

#### Compilation

When compiling the Allpix Squared framework, the TCAD DF-ISE mesh converter is automatically compiled and installed in the Allpix Squared installation directory.

It is also possible to compile the converter separately as stand-alone tool within this directory:
```bash
$ mkdir build && cd build
$ cmake ..
$ make
```

It should be noted that the TCAD DF-ISE mesh converter depends on the core utilities of the Allpix Squared framework found in the directory `src/core/utils`. Thus, it is discouraged to move the converter code outside the repository as this directory would have to be copied and included in the code as well. Furthermore, updates are only distributed through the repository and new release versions of the Allpix Squared framework.

#### Features
- TCAD DF-ISE file format reader.
- Fast radius neighbor search for three-dimensional point clouds.
- Barycentric interpolation between non-regular mesh points.
- Several cuts available on the interpolation algorithm variables.
- Interpolated data visualization tool.

#### Usage
To run the program, the following command should be executed from the installation folder:
```bash
bin/tcad_dfise_converter/dfise_converter -f <file_name_prefix> [<options>] [<arguments>]
```
The list with options can be accessed using the -h option.
Possible options and their default values are:
```
-f <file_prefix>       common prefix of DF-ISE grid (.grd) and data (.dat) files
-o <init_file_prefix>  output file prefix without .init (defaults to file name of <file_prefix>)
-R <region>            region name to be meshed (defaults to 'bulk')
-O <observable>        observable to be interpolated (defaults Electric Field)
-r <radius>            initial node neighbors search radius in um (defaults to 1 um)
-t <radius_threshold>  minimum distance from node to new mesh point (defaults to 0 um)
-s <radius_step>       radius step if no neighbor is found (defaults to 0.5 um)
-m <max_radius>        maximum search radius (default is 10 um)
-i <index_cut>         index cut during permutation on vertex neighbours (disabled by default)
-c <volume_cut>        minimum volume for tetrahedron for non-coplanar vertices (defaults to minimum double value)
-x <mesh x_pitch>      new regular mesh X pitch (defaults to 100)
-y <mesh_y_pitch>      new regular mesh Y pitch (defaults to 100)
-z <mesh_z_pitch>      new regular mesh Z pitch (defaults to 100)
-d <mesh_dimension>    specify mesh dimensionality (defaults to 3)
-l <file>              file to log to besides standard output (disabled by default)
-v <level>             verbosity level (default reporiting level is INFO)
```

Observables currently implemented for interpolation are: *ElectrostaticPotential*, *ElectricField*, *DopingConcentration*, *DonorConcentration* and *AcceptorConcentration*.
The output INIT file will be saved with the same *file_name_prefix* as the .grd and .dat files, +*_observable_interpolated.init*.

The *mesh_plotter* tool can be used from the installation folder as follows:
```bash
bin/tcad_dfise_converter/mesh_plotter -f <file_name> [<options>] [<arguments>]
```
The list with options and defaults is displayed with the -h option.
In a 3D mesh, the plane to be plotted must be identified by using the option -p with argument *xy*, *yz* or *zx*, defaulting to *yz*.
The data to be plotted can be selected with the -d option, the arguments are *ex*, *ey*, *ez* for the vector components or the default value *n* for the norm of the electric field.


#### Octree
J. Behley, V. Steinhage, A.B. Cremers. *Efficient Radius Neighbor Search in Three-dimensional Point Clouds*, Proc. of the IEEE International Conference on Robotics and Automation (ICRA), 2015 [@octree].

Copyright 2015 Jens Behley, University of Bonn.
This project is free software made available under the MIT License. For details see the LICENSE.md file.

[@octree]: http://jbehley.github.io/papers/behley2015icra.pdf
