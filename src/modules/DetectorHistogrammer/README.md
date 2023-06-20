---
# SPDX-FileCopyrightText: 2017-2023 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "DetectorHistogrammer"
description: "Provisional simulation analysis for a detector"
module_status: "Functional"
module_maintainers: ["Paul Schuetze (<paul.schuetze@desy.de>)", "Simon Spannagel (<simon.spannagel@cern.ch>)"]
module_outputs: ["PixelHit", "MCParticle"]
---

## Description
This module provides an overview of the produced simulation data for a quick inspection and simple checks.
For more sophisticated analyses, the output from one of the output writers should be used to make the necessary information available.

Within the module, clustering of the input hits is performed.
Looping over the PixelHits, hits being adjacent to an existing cluster are added to this cluster.
Clusters are merged if there are multiple adjacent clusters.
If the PixelHit is free-standing, a new cluster is created.

This module serves as a quick "mini-analysis" and creates the histograms listed below.
The Monte Carlo truth position provided by the `MCParticle` objects is used as track reference position.
An additional uncertainty can be added by configuring a track resolution, with which every cluster residual is convolved. This makes it possible to perform a quick test beam-like analysis.
For technical reasons, this offset is drawn randomly from a Gaussian distribution independently for the resolution and the efficiency measurement. **Note:** If a non-zero track resolution is used, pixel matrix edge effects may appear as particles hit the sensor excess.

* A hitmap of all pixels in the pixel grid, displaying the number of times a pixel has been hit during the simulation run.
* A cluster map indicating the cluster positions for the whole simulation run.
* Distribution of the total number of pixel hits (event size) per event.
* Distribution of the total number of clusters found per event.
* Distributions of the cluster size in x, y and the total cluster size.
* Mean cluster size and cluster sizes in x and y as function of the in-pixel impact position of the primary particle.
* Residual distribution in x and y between the center-of-gravity position of the cluster and the primary particle.
* Residual map for residuals in x, y, and combined between the center-of-gravity position of the cluster and the primary particle. These maps allow to see if the residuals are smaller or larger on some part of the detector compared to others.
* Mean absolute deviations of the residual as function of the in-pixel impact position of the primary particle. Histograms both for a 2D representation of the pixel cell as well as the projections (residual X vs position X, residual Y vs position Y, residual X vs position Y, residual Y vs position X) are produced.
* Efficiency map of the detector
* Efficiency as function of the in-pixel impact position of the primary particle. Histograms both for a 2D representation of the pixel cell as well as the projections (efficiency vs position X, efficiency vs position Y) are produced.
* Total cluster, pixel and event charge distributions.
* Mean total cluster charge as function of the in-pixel impact position of the primary particle.
* Mean seed pixel charge as a function  of the in-pixel impact position of the primary particle.

## Parameters

* `granularity`: 2D integer vector defining the number of bins along the *x* and *y* axis for in-pixel maps. Defaults to the pixel pitch in micro meters, e.g. a detector with 100um x 100um pixels would be represented in a histogram with `100 * 100 = 10000` bins.
* `granularity_local`: 2D integer vector defining the number of bins for each pixel along the *x* and *y* axis for maps in local coordinates where particle positions are used as reference. Defaults to `1 1` corresponding to a single bin per pixel.
* `max_cluster_charge`: Upper limit for the cluster charge histogram, defaults to `50ke`.
* `track_resolution`: Assumed track resolution the Monte Carlo truth is smeared with. Expects two values for the resolution in local-x and local-y directions and defaults to `0um 0um`, i.e. no smearing.
* `matching_cut`: Required maximum matching distance between cluster position and particle position for the efficiency measurement. Expected two values and defaults to three times the pixel pitch in each dimension.

## Usage
This module is normally bound to a specific detector to plot, for example to the 'dut':

```ini
[DetectorHistogrammer]
name = "dut"
granularity = 100, 100
```
