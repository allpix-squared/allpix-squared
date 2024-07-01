---
# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Mesh Converter"
---

This code takes adaptive meshes from finite-element simulations and transforms them into a regularly spaced grid for faster
field value lookup as required by Monte Carlo simulations tools such as Allpix Squared. The input consists of two files, one
containing the vertex coordinates of each input mesh node, the other providing the relevant field values associated to each
of these vertices. One output file containing the regular mesh is produced. This tool can work in two different modes, the
closest-neighbor mode and interpolation mode:

### Simple Closest-Neighbor Search

In this mode, selected by setting the parameter `interpolate = false`, no interpolation of field values is performed, but for
every output mesh point, the values from the closest neighbor of the input mesh are taken. In most cases this approach should
produces reasonably precise results with a granularity similar to the respective adaptive mesh granularity in the respective
region. The tool uses the Octree `findNeighbor` algorithm \[[@octree]\] to find the closest neighbor to the query point.

### Barycentric Interpolation Method

In this mode, the regular mesh is created by scanning the model volume in regular X, Y and Z steps and using a barycentric
interpolation method to calculate the respective electric field vector on the new point. The interpolation uses the four
closest, no-coplanar, neighbor vertex nodes such, that the respective tetrahedron encloses the query point. For the neighbors
search, the tool uses the Octree `radiusNeighbors` neighbor search algorithm \[[@octree]\].

## File Formats

### Input Data

Currently, this tool supports the TCAD DF-ISE data format as well as output generated from Silvaco TCAD. The respective
parser has to be chosen using the `parser` configuration parameter.

For the DF-ISE format, the mesh converter requires the `.grd` and `.dat` files as input alongside the parameter `parser = df-ise`.
Here, the `.grd` file contains the vertex coordinates (3D or 2D) of each mesh node and the `.dat` file contains the value of each electric field vector component for each mesh node, grouped by model regions (such as silicon bulk or metal contacts). The regions are defined in the `.grd` file by grouping vertices into edges, faces and, consecutively, volumes or elements.

For Silvaco TCAD, the data has to be extracted from the TCAD data, and `parser = silvaco` has to be selected. The required input files are a `.grd` file containing the list of mesh points and `.dat` file holding the corresponding values.


### Output Data

This tools can produce output in two different formats, with the file extensions `.init` and `.apf`.
Both file formats can be imported into Allpix Squared.

The **APF** (Allpix Squared Field) data format contains the field data in binary form and is therefore a bit more compact and can be read much faster. Whenever possible, this format should be preferred.

The **INIT** file is an ASCII text file with a format used by other tools such as PixelAV.
Its header therefore contains several fields which are not used by Allpix Squared but need to be present nevertheless. The following example shows such a file header, important variables are marked with `<...>` while other fields are not interpreted and can be left untouched:

```ini
<first line: some descriptive text to identify the field or field source>
##SEED##  ##EVENTS##
##TURN## ##TILT## 1.0
0.00 0.0 0.00
<thickness in um> <size x in um> <size y in um> 0 0
0 0 <number of bins x> <number of bins y> <number of bins z> 0
```

After the header part, the data follows as list of individual nodes with three indices for `x`, `y`, and `z` coordinates at the beginning and the scalar or vector field components afterwards. For a vector field, this looks like:

```ini
<node.x> <node.y> <node.z> <observable.x> <observable.y> <observable.z>
```

whereas for a scalar field such as a weighting potential, only one field component is present:

```ini
<node.x> <node.y> <node.z> <observable>
```


## Compilation

When compiling the Allpix Squared framework, the Mesh Converter is automatically compiled and installed in the Allpix Squared installation directory.

It is also possible to compile the converter separately as stand-alone tool within this directory:

```shell
mkdir build
cd build
cmake ..
make
```

It should be noted that the Mesh Converter depends on the core utilities of the Allpix Squared framework found in the directory `src/core/utils`. Thus, it is discouraged to move the converter code outside the repository as this directory would have to be copied and included in the code as well. Furthermore, updates are only distributed through the repository and new release versions of the Allpix Squared framework.

## Features

* TCAD DF-ISE file format parser.
* Automatic determination of the input mesh dimensionality (2D/3D).
* Fast radius neighbor search for three-dimensional point clouds.
* Barycentric interpolation between non-regular mesh points.
* Several cuts available on the interpolation algorithm variables.
* Interpolated data visualization tool.

### Parameters

* `model`: Field file format to use, can be **INIT** or **APF**, defaults to **APF** (binary format).
* `parser`: Parser class to interpret input data in. Supported values are **DF-ISE** (default) and **Silvaco**.
* `region`: Region name or list of region names to be meshed, such as `bulk` or `"bulk","epi"` (No default value; required parameter).
* `observable`: Observable to be interpolated, such as `ElectricField` (No default value; required parameter).
* `observable_units`: Units in which the observable is stored in the input file (No default value; required parameter).
* `interpolate`: Boolean switch to select either the barycentric interpolation method or the closest-neighbor method. Defaults to `true`, i.e. using the interpolation method.
* `initial_radius`: Initial node neighbors search radius in micro meters. Defaults to the minimal cell dimension of the final interpolated mesh.
* `radius_step`: Radius step if no neighbor is found (defaults to `0.5um`). Only used for barycentric interpolation.
* `max_radius`: Maximum search radius (default is `50um`). Only used for barycentric interpolation.
* `allow_coplanar_interpolation`: Allow the interpolation to use coplanar/colinear vertices if no full interpolation volume can be found after increasing the search radius and if more than 100 neighbors are found. Defaults to `false`. It should be noted that this feature is experimental and that it can produce `NaN` results for the interpolated field.
* `allow_failure`: Allow the interpolation of a single mesh point to fail, i.e. when no neighbors could be found. If set to `true`, the respective mesh element will be set to zero and the interpolation will continue, if `false` the interpolation will be aborted. Defaults to `false`. Only used for barycentric interpolation.
* `volume_cut`: Minimum volume for tetrahedron for non-coplanar vertices (defaults to minimum double value). Only used for barycentric interpolation.
* `divisions`: Number of divisions of the new regular mesh for each dimension, 2D or 3D vector depending on the `dimension` setting. Defaults to 100 bins in each dimension.
* `xyz`: Array to replace the system coordinates of the mesh. A detailed description of how to use this parameter is given below.
* `workers`: Number of worker threads to be used for the interpolation. Defaults to the available number of cores on the machine (hardware concurrency).
* `vector_field`: Select if the observable is a vector field or scalar field (Defaults to `true` matching the default observable `ElectricField`).
* `log_level`: Specifies the lowest log level which should be reported. Possible values are the same as for the Allpix Squared framework.

### Usage

To run the program, the following command should be executed from the installation folder:

```shell
mesh_converter -f <file_prefix> [<options>] [<arguments>]
```

The converter will look for a configuration file with `<file_prefix>` and `.conf` extension. This default configuration file name can be replaced with the `-c` option.
The list with options can be accessed using the `-h` option.
Possible options and their default values are:

```shell
-f <file_prefix>       common prefix of DF-ISE grid (.grd) and data (.dat) files
-c <config_file>       configuration file setting mesh conversion parameters
-h                     display this help text
-l <file>              file to log to besides standard output (disabled by default)
-o <init_file_prefix>  output file prefix without .init (defaults to file name of <file_prefix>)
-v <level>             verbosity level (default reporting level is INFO)
```

Observables currently implemented for interpolation are: `ElectrostaticPotential`, `ElectricField`, `DopingConcentration`, `DonorConcentration` and `AcceptorConcentration`.
The output INIT/APF file will be saved with the same file_prefix as the `.grd` and `.dat` files and the additional name suffix `_<observable>_interpolated` and the appropriate file extension, where `<observable>` is replaced with the selected quantity.

The new coordinate system of the mesh can be changed by providing an array for the *xyz* keyword in the configuration file. The first entry of the array, representing the new mesh *x* coordinate, should indicate the TCAD original mesh coordinate (*x*, *y* or *z*), and so on for the second (*y*) and third (*z*) array entry. For example, if one wants to have the TCAD *x*, *y* and *z* mesh coordinates mapped into the *y*, *z* and *x* coordinates of the new mesh, respectively, the configuration file should have `xyz = z x y`. If one wants to flip one of the coordinates, the minus symbol (`-`) can be used in front of one of the coordinates (such as `xyz = z x -y`).

The program can be used to convert 3D and 2D TCAD mesh files. Note that when converting 2D meshes, the *x* coordinate will be fixed to 1 and the interpolation will happen over the *yz* plane.
The keyword mesh_tree can be used as a switch to enable or disable the creation of a root file with the original TCAD mesh points stored as a `ROOT::TTree` for later, fast, inspection.


## Mesh Plotter

In addition to the Mesh Converter, the `mesh_plotter` tool can be used to visualize the new mesh interpolation results, from the installation folder as follows:

```shell
mesh_plotter -f <file_name> [<options>] [<arguments>]
```

The following command-line options are supported:

```shell
-f <file_name>         name of the interpolated file in APF or INIT format
-c <cut>               projection height index (default is mesh_pitch / 2)
-h                     display this help text
-l                     plot with logarithmic scale if set
-o <output_file_name>  name of the file to output (default is efield.png)
-p <plane>             plane to be plotted. xy, yz or zx (default is yz)
-u <units>             units to interpret the field data in
-s                     parsed observable is a scalar field
```

The list with options and defaults is displayed with the `-h` option.
In a 3D mesh, the plane to be plotted must be identified by using the option `-p` with argument *xy*, *yz* or *zx*, defaulting to *yz*.
By default, the data is interpreted as a vector field, where graphs for all three components are created.
Using the option `-s` enables the interpretation of a scalar field.
The units for the field to interpreted in can be defined via the option `-u`.
The number of mesh divisions in each dimension is automatically read from the `init`/`apf` file, by default the cut in the third dimension is done in the center but can be shifted using the `-c` option described above.

[@octree]: http://jbehley.github.io/papers/behley2015icra.pdf
