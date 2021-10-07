---
# SPDX-FileCopyrightText: 2022 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Field Maps"
weight: 6
---

Allpix Squared allows to load externally generated field maps for various quantities such as the electric field or the doping
profile of the sensor. These maps have to be provided as regularly-spaced meshes in one of the supported field file formats.
A conversion and interpolation tool to translate adaptive-mesh fields from TCAD applications to the format required by Allpix
Squared is provided together with the framework and is described in [Section 13.2](../13_additional/mesh_converter.md).

![](./maps_types.png)\
*Examples for pixel geometries in field maps. The dark spot represents the pixel center, the red extend the electric field.
Pixel boundaries are indicated with a dotted line where applicable.*

Fields are always expected to be provided as rectangular maps, irrespective of the actual pixel shape.
Maps are loaded once and assigned on a per-pixel basis.
Depending on the symmetries of the pixel unit cell and the pixel grid, different geometries are supported as indicated in the
figure above. The field for a quarter of the pixel plane, for half planes (see figures below) as well as for full planes
(see figure above). The size of the field is not limited to a single pixel cell, however, for some quantities such as the
electric field only the volume within the pixel cell is used and periodic boundary conditions are assumed and expected.
Larger fields are for example useful for the weighting potential, where also potential differences to neighboring pixels
are of interest.

A special case is the field presented in the right panel of the figure above. Here, the field is not centered at the pixel
unit cell center, but at the corner of four adjacent rectangular pixels.

![](./maps_half.png)\
*Location and orientation of the field map with respect to the pixel center when providing a half of the pixel plane. Here,
$`(0,0)`$ denotes the pixel center, the red field portion is read from the field map and the green ones are replicated
through mirroring.*

![](./maps_quadrant.png)\
*Location and orientation of the field map with respect to the pixel center when providing one quadrant in the pixel plane.
Here, $`(0,0)`$ denotes the pixel center, the red field portion is read from the field map and the green ones are replicated
through mirroring.*


The parameter `field_mapping` of the respective module defines how the map read from the mesh file should be interpreted and
applied to the cell, and the following possibilities are available:

- `FULL`:
  The map is interpreted as field spanning the full Euclidean angle and aligned on the center of the pixel unit cell. No
  transformation is performed, but field values are obtained from the map with respect to the pixel center.

- `FULL_INVERSE`:
  The map is interpreted as full field, but with inverse alignment on the pixel corners as shown above. Consequently, the
  field value lookup from the four quadrants take into account their offset.

- `HALF_LEFT`:
  The map represents the left Euclidean half-plane, aligned at the $`y`$ axis through the center of the pixel unit cell.
  Field values in the other half-plane are obtained by mirroring at the $`y`$ axis as indicated in the figure above.

- `HALF_RIGHT`:
  The map represents the right Euclidean half-plane, aligned at the $`y`$ axis through the center of the pixel unit cell.
  Field values in the other half-plane are obtained by mirroring at the $`y`$ axis as indicated in the figure above.

- `HALF_TOP`:
  The map represents the top Euclidean half-plane, aligned at the $`x`$ axis through the center of the pixel unit cell. Field
  values in the other half-plane are obtained by mirroring at the $`x`$ axis as indicated in the figure above.

- `HALF_BOTTOM`:
  The map represents the bottom Euclidean half-plane, aligned at the $`x`$ axis through the center of the pixel unit cell.
  Field values in the other half-plane are obtained by mirroring at the $`x`$ axis as indicated in the figure above.

- `QUADRANT_I`:
  The map represents the quadrant of the plane where both vector components have a positive sign. Field values in the other
  three quadrants are obtained by mirroring at the axes of the coordinate system as show in the figure above.

- `QUADRANT_II`:
  The map represents the quadrant of the plane where the vector component $`x`$ has a negative and $`y`$ a positive sign.
  Field values in the other three quadrants are obtained by mirroring at the axes of the coordinate system as show in the
  figure above.

- `QUADRANT_III`:
  The map represents the quadrant of the plane where both vector components have a negative sign. Field values in the other
  three quadrants are obtained by mirroring at the axes of the coordinate system as show in the figure above.

- `QUADRANT_IV`:
  The map represents the quadrant of the plane where the vector component $`x`$ has a positive and $`y`$ a negative sign.
  Field values in the other three quadrants are obtained by mirroring at the axes of the coordinate system as show in the
  figure above.


All axes mentioned here are Cartesian axes aligning with the local coordinate system of the sensor, described in
[Section 4.5](./05_geometry_detectors.md#coordinate-systems), and passing through the center of the pixel unit cell regarded.
It should be noted that some of these mappings are equivalent to rotating or mirroring the field before loading it in
Allpix Squared, and are only provided for convenience.
