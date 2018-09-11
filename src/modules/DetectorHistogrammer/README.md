# DetectorHistogrammer
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>), Paul Schuetze (<paul.schuetze@desy.de>), Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Functional  
**Input**: PixelHit, MCParticle

### Description
This module provides an overview of the produced simulation data for a quick inspection and simple checks. For more sophisticated analyses, the output from one of the output writers should be used to make the necessary information available.

Within the module, clustering of the input hits is performed. Looping over the PixelHits, hits being adjacent to an existing cluster are added to this cluster. Clusters are merged if there are multiple adjacent clusters. If the PixelHit is free-standing, a new cluster is created.

This module serves as a quick "mini-analysis" and creates the histograms listed below.
If no MCParticle information is available, some of the plots might not be filled.

* A hitmap of all pixels in the pixel grid, displaying the number of times a pixel has been hit during the simulation run.
* A cluster map indicating the cluster positions for the whole simulation run.
* Distribution of the total number of pixel hits (event size) per event.
* Distribution of the total number of clusters found per event.
* Distributions of the cluster size in x, y and the total cluster size.
* Mean cluster size and cluster sizes in x and y as function of the in-pixel impact position of the primary particle.
* Residual distribution in x and y between the center-of-gravity position of the cluster and the primary particle.
* Mean absolute deviations of the residual as function of the in-pixel impact position of the primary particle.
* Total cluster charge distribution.
* Mean total cluster charge as function of the in-pixel impact position of the primary particle.
* Mean seed pixel charge as a function  of the in-pixel impact position of the primary particle.

### Parameters

* `granularity`: 2D integer vector defining the number of bins along the *x* and *y* axis for in-pixel maps. Defaults to the pixel pitch in micro meters, e.g. a detector with 100um x 100um pixels would be represented in a histogram with `100 * 100 = 10000` bins.

### Usage
This module is normally bound to a specific detector to plot, for example to the 'dut':

```ini
[DetectorHistogrammer]
name = "dut"
granularity = 100, 100
```
