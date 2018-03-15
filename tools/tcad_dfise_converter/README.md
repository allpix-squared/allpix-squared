# TCAD DF-ISE mesh converter
This code takes the .grd and .dat files of the DF-ISE format from TCAD simulations as input. The .grd file contains the vertex coordinates (3D or 2D) of each mesh node and the .dat file contains the value of each electric field vector component for each mesh node, grouped by model regions (such as silicon bulk or metal contacts). The regions are defined in the .grd file by grouping vertices into edges, faces and, consecutively, volumes or elements.

A new regular mesh is created by scanning the model volume in regular X Y and Z steps (not necessarily coinciding with original mesh nodes) and using a barycentric interpolation method to calculate the respective electric field vector on the new point. The interpolation uses the four closest, no-coplanar, neighbor vertex nodes such, that the respective tetrahedron encloses the query point. For the neighbors search, the software uses the Octree implementation [@octree].

The output .init file (with the same name as the .grd and .dat files) can be imported into Allpix Squared. The INIT file has a header followed by a list of columns organized as
```bash
node.x	node.y	node.z	observable.x	observable.y	observable.z
```

### Compilation

When compiling the Allpix Squared framework, the TCAD DF-ISE mesh converter is automatically compiled and installed in the Allpix Squared installation directory.

It is also possible to compile the converter separately as stand-alone tool within this directory:
```bash
$ mkdir build && cd build
$ cmake ..
$ make
```

It should be noted that the TCAD DF-ISE mesh converter depends on the core utilities of the Allpix Squared framework found in the directory `src/core/utils`. Thus, it is discouraged to move the converter code outside the repository as this directory would have to be copied and included in the code as well. Furthermore, updates are only distributed through the repository and new release versions of the Allpix Squared framework.

### Features
- TCAD DF-ISE file format reader.
- Fast radius neighbor search for three-dimensional point clouds.
- Barycentric interpolation between non-regular mesh points.
- Several cuts available on the interpolation algorithm variables.
- Interpolated data visualization tool.

### Parameters
* `dimension`: Specify mesh dimensionality (defaults to 3).
* `region`: Region name to be meshed (defaults to 'bulk').
* `observable`: Observable to be interpolated (defaults Electric Field).
* `radius`: Initial node neighbors search radius in um (defaults to 1 um).
* `radius_step`: Radius step if no neighbor is found (defaults to 0.5 um).
* `max_radius`: Maximum search radius (default is 10 um).
* `radius_threshold`: Minimum distance from node to new mesh point (defaults to 0 um).
* `volume_cut`: Minimum volume for tetrahedron for non-coplanar vertices (defaults to minimum double value).
* `index_cut`: Index cut during permutation on vertex neighbours (disabled by default).
* `xdiv`: New regular mesh X pitch (defaults to 100 for 3D mesh, and 1 for 2D mesh).
* `ydiv`: New regular mesh Y pitch (defaults to 100).
* `zdiv`: New regular mesh Z pitch (defaults to 100).
* `xyz`: Array to replace the system coordinates of the mesh. 
* `screen_shot`: Enables "screen-shot" of mesh points, point being interpolated (in red) and neighbouring pixels (in blue) (defaults to -1 -1 -1, disabling the screen-shot). 
* `ss_radius`: Sets a region of interest around the point being interpolated to show the mesh points.
* `mesh_tree`: Switch enabling the creation of a root file with the TCAD mesh nodes stored in a ROOT::TTree (automatically enabled if screen-shot is activated).

### Usage
To run the program, the following command should be executed from the installation folder:
```bash
bin/tcad_dfise_converter/dfise_converter -f <file_prefix> [<options>] [<arguments>]
```
The converter will look for a configuration file with <file_prefix> and .conf extension. The configuration file can be repaced with the -c option.
The list with options can be accessed using the -h option.
Possible options and their default values are:
```
-f <file_prefix>       common prefix of DF-ISE grid (.grd) and data (.dat) files
-c <config_file>	   configuration file seting mesh conversion parameters
-o <init_file_prefix>  output file prefix without .init (defaults to file name of <file_prefix>)
-l <file>              file to log to besides standard output (disabled by default)
-v <level>             verbosity level (default reporiting level is INFO)
```

Observables currently implemented for interpolation are: *ElectrostaticPotential*, *ElectricField*, *DopingConcentration*, *DonorConcentration* and *AcceptorConcentration*.
The output INIT file will be saved with the same *file_prefix* as the .grd and .dat files, +*_observable_interpolated.init*.

The new coordinate system of the mesh can be changed by providing an array for the *xyz* keyword in the configuration file. The first entry of the array, representing the new mesh *x* coordinate, should indicate the TCAD original mesh coordinate (*x*, *y* or *z*), and so on for the second (*y*) and third (*z*) array entry. For example, if one wants to have the TCAD *x*, *y* and *z* mesh coordinates mapped into the *y*, *z* and *x* coordinates of the new mesh, respectiviely, the configuration file should have *xyz = z x y*. If one wants to flip one of the coordinates, the minus symbol ("-") can be used in front of one of the coordinates (such as *xyz = z x -y*).  

The program can be used to convert 3D and 2D TCAD mesh files. Note that when converting 2D meshes, the *x* coordinate will be fixed to 1 and the interpolation will happen over the *yz* plane.
The keyword mesh_tree can be used as a switch to enable or disable the creation of a root file with the original TCAD mesh points stored as a ROOT::TTree for later, fast, inspection.

It is possible to visualise the position of the new mesh point to be interpolated (in red) surounded by the mesh points and the closest neighbours found (in blue) by providing the keyword *screen_shot* with the new mesh node coordinates (such as *screen_shot = 1 2 3*) as value pair. This can be useful if the interpolation gets stuck in some region. Currently, it is implemented only for 3D meshes. The 3D point-cloud will be saved as a ROOT::TGraph2D in a root file with the grid file name (including .grd extension) as prefix and "_INTERPOLATION_POINT_SCREEN_SHOT.root" as sufix.

In addition, the *mesh_plotter* tool can be used, in order to visualise the new mesh interpolation results, from the installation folder as follows:
```bash
bin/tcad_dfise_converter/mesh_plotter -f <file_name> [<options>] [<arguments>]
```
The list with options and defaults is displayed with the -h option.
In a 3D mesh, the plane to be plotted must be identified by using the option -p with argument *xy*, *yz* or *zx*, defaulting to *yz*.
The data to be plotted can be selected with the -d option, the arguments are *ex*, *ey*, *ez* for the vector components or the default value *n* for the norm of the electric field.


### Octree
J. Behley, V. Steinhage, A.B. Cremers. *Efficient Radius Neighbor Search in Three-dimensional Point Clouds*, Proc. of the IEEE International Conference on Robotics and Automation (ICRA), 2015 [@octree].

Copyright 2015 Jens Behley, University of Bonn.
This project is free software made available under the MIT License. For details see the LICENSE.md file.

[@octree]: http://jbehley.github.io/papers/behley2015icra.pdf
