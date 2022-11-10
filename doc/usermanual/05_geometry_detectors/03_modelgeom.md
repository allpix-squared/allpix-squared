---
# SPDX-FileCopyrightText: 2022 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Sensor Geometries"
weight: 3
---

Allpix Squared implements different sensor geometries such as rectangular, Cartesian pixel grids, hexagon patterns, or radial
strip-like channels. This section details the different geometries and their respective coordinate system. Geometries are
selected via the parameter `geometry` in the detector model file.

## Rectangular Pixels on a Cartesian Grid

This geometry is the default assumed for any detector without the `geometry` keyword. The individual channels are rectangular
pixels, the `pixel_size` parameter denotes the pitch in Cartesian `x` and `y` direction.

This geometry can be selected using `geometry = pixel`


## Hexagonal Pixels

Hexagonal pixel grids in Allpix Squared use an axial coordinate system to describe the relative positions and indices of
hexagons on the grid, following largely the definitions provided in \[[@hexagons]\]. Similar to the Cartesian coordinate
system used for regular pixel layouts, the origin is the lower-left corner of the sensor, with the hexagon indices $`(0,0)`$.
Owing to the orientation of the grid axes, negative can occur in the top-left region of the sensor.

Two orientations of hexagons are supported, subsequently referred to as *pointy* with sides parallel to the $`y`$ axis of the
Cartesian coordinate system and corners at the top and bottom, and *flat* with sides parallel to the Cartesian $`x`$ axis and
corners to the left and right. The pitches $`p_x`$ and $`p_y`$ of the hexagon align with the axial coordinate system and are
rotated differently with respect to the Cartesian system between the two variants. The orientation of the pitches as well as
the resulting corner positions in Cartesian coordinates are shown in the figure below:

![](./hexagon_orientations.png)\
*Definition of the pitches $`p_x`$ and $`p_y`$, and corner positions for the pointy (left) and flat (right) hexagon
orientation in Cartesian coordinates. The pitches align with the axes of the axial coordinate system of the hexagonal grid.*

The additional parameters for the **hexagonal** model are as follows:

- `pixel_type`:
   The shape/orientation of the hexagonal pixels within the grid, either `hexagon_pointy` or `hexagon_flat`.

The number of pixels in a hexagonal grid are counted along the Cartesian axes, taking the offset pixels into account.
For example, an 8-by-4 grid comprises 32 pixels both for *pointy* and *flat* hexagon orientation, but results in different
overall grid dimensions as demonstrated below:

![](./hexagon_grids.png)\
*Grid layouts for pointy (left) and flat (right) hexagons with a size of 8-by-4 pixels.*

This geometry can be selected using `geometry = hexagon`


## Radial Strips



[@hexagons]: https://www.redblobgames.com/grids/hexagons/
